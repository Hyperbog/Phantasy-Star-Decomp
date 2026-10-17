#include "modernps/economy.hpp"

#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace modernps;
namespace {
void check(bool good, const char* message) { if (!good) throw std::runtime_error(message); }
template <typename Error, typename Fn> void rejects(Fn&& fn) {
    bool threw = false;
    try { fn(); } catch (const Error&) { threw = true; }
    check(threw, "expected error not raised");
}
ItemCatalog catalog() {
    ItemCatalog items;
    items.add({"tonic", "Tonic", ItemKind::consumable, 5, 10, 0, 0, 0});
    items.add({"blade", "Blade", ItemKind::weapon, 1, 0, 2, 0, 0});
    items.add({"quest_key", "Quest Key", ItemKind::key_item, 1, 0, 0, 0, 0});
    return items;
}
Shop shop(const ItemCatalog& cat) {
    return Shop({"merchant", {{"tonic", 12, 6}, {"blade", 40, 20}}}, cat);
}
void purchases_and_sales() {
    auto cat = catalog(); auto market = shop(cat);
    Wallet wallet(100); Inventory items(2);
    market.buy("tonic", 2, wallet, items, cat);
    check(wallet.balance() == 76 && items.count("tonic") == 2, "purchase incorrect");
    market.sell("tonic", 1, wallet, items, cat);
    check(wallet.balance() == 82 && items.count("tonic") == 1, "sale incorrect");
    market.buy("blade", 1, wallet, items, cat);
    check(wallet.balance() == 42 && items.count("blade") == 1, "second purchase wrong");
}
void insufficient_funds_are_atomic() {
    auto cat = catalog(); auto market = shop(cat);
    Wallet wallet(11); Inventory items;
    rejects<std::invalid_argument>([&] { market.buy("tonic", 1, wallet, items, cat); });
    check(wallet.balance() == 11 && items.count("tonic") == 0, "insufficient funds mutated state");
}
void inventory_failure_is_atomic() {
    auto cat = catalog(); auto market = shop(cat);
    Wallet wallet(100); Inventory items(1);
    items.add(cat, "blade", 1);
    rejects<std::invalid_argument>([&] { market.buy("tonic", 1, wallet, items, cat); });
    check(wallet.balance() == 100 && items.count("tonic") == 0 &&
          items.count("blade") == 1, "inventory failure charged buyer");
}
void sale_overflow_is_atomic() {
    auto cat = catalog(); auto market = shop(cat);
    Wallet wallet(Wallet::max_money); Inventory items;
    items.add(cat, "tonic", 1);
    rejects<std::invalid_argument>([&] { market.sell("tonic", 1, wallet, items, cat); });
    check(wallet.balance() == Wallet::max_money && items.count("tonic") == 1,
          "sale overflow removed items");
}
void invalid_quantities_and_offers() {
    auto cat = catalog(); auto market = shop(cat);
    Wallet wallet(100); Inventory items;
    rejects<std::invalid_argument>([&] { market.buy("tonic", 0, wallet, items, cat); });
    rejects<std::invalid_argument>([&] { market.buy("tonic", std::numeric_limits<int>::max(), wallet, items, cat); });
    rejects<std::invalid_argument>([&] { market.sell("tonic", 1, wallet, items, cat); });
    rejects<std::out_of_range>([&] { market.buy("quest_key", 1, wallet, items, cat); });
    rejects<std::invalid_argument>([&] { Shop bad({"bad", {{"quest_key", 3, 1}}}, cat); });
    rejects<std::invalid_argument>([&] { Shop bad({"bad", {{"tonic", 3, 4}}}, cat); });
    rejects<std::invalid_argument>([&] { Shop bad({"bad", {{"tonic", 3, 1}, {"tonic", 3, 1}}}, cat); });
    check(wallet.balance() == 100 && items.slots_used() == 0, "invalid operation mutated state");
}
void wallet_limits() {
    rejects<std::invalid_argument>([] { Wallet w(-1); });
    rejects<std::invalid_argument>([] { Wallet w(Wallet::max_money+1); });
    Wallet w(1);
    rejects<std::invalid_argument>([&] { w.debit(2); });
    rejects<std::invalid_argument>([&] { w.credit(Wallet::max_money); });
    rejects<std::invalid_argument>([&] { w.credit(0); });
    check(w.balance() == 1, "wallet invalid operation mutated state");
}
} // namespace
int main() {
    const std::pair<const char*, std::function<void()>> tests[] = {
        {"purchases_and_sales", purchases_and_sales},
        {"insufficient_funds_are_atomic", insufficient_funds_are_atomic},
        {"inventory_failure_is_atomic", inventory_failure_is_atomic},
        {"sale_overflow_is_atomic", sale_overflow_is_atomic},
        {"invalid_quantities_and_offers", invalid_quantities_and_offers},
        {"wallet_limits", wallet_limits},
    };
    try { for (auto [name, fn] : tests) { fn(); std::cout << "PASS " << name << '\n'; } }
    catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
    return 0;
}
