#include "modernps/economy.hpp"

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace modernps {

Wallet::Wallet(int money) : money_(money) {
    if (money < 0 || money > max_money) throw std::invalid_argument("invalid wallet balance");
}

void Wallet::credit(int amount) {
    if (amount <= 0 || static_cast<std::int64_t>(money_) + amount > max_money)
        throw std::invalid_argument("wallet overflow or invalid credit");
    money_ += amount;
}

void Wallet::debit(int amount) {
    if (amount <= 0 || amount > money_)
        throw std::invalid_argument("insufficient funds or invalid debit");
    money_ -= amount;
}

void Wallet::swap(Wallet& other) noexcept { std::swap(money_, other.money_); }

Shop::Shop(ShopDefinition definition, const ItemCatalog& catalog)
    : definition_(std::move(definition)) {
    if (!valid_content_id(definition_.id) || definition_.offers.empty() ||
        definition_.offers.size() > 32) throw std::invalid_argument("invalid shop");
    for (const auto& offer : definition_.offers) {
        if (!catalog.contains(offer.item_id) ||
            catalog.get(offer.item_id).kind == ItemKind::key_item ||
            offer.buy_price < 1 || offer.buy_price > Wallet::max_money ||
            offer.sell_price < 0 || offer.sell_price > offer.buy_price ||
            !offers_.emplace(offer.item_id, offer).second)
            throw std::invalid_argument("invalid or duplicate shop offer");
    }
}

const ShopOffer& Shop::offer(std::string_view id) const {
    auto it = offers_.find(id);
    if (it == offers_.end()) throw std::out_of_range("item not sold by shop");
    return it->second;
}

void Shop::buy(std::string_view id, int quantity, Wallet& money,
               Inventory& inventory, const ItemCatalog& catalog) const {
    const auto& entry = offer(id);
    if (quantity <= 0) throw std::invalid_argument("invalid purchase quantity");
    const auto cost = static_cast<std::int64_t>(entry.buy_price) * quantity;
    if (cost > Wallet::max_money) throw std::invalid_argument("purchase cost overflow");
    Wallet staged_money = money;
    Inventory staged_inventory = inventory;
    staged_money.debit(static_cast<int>(cost));
    staged_inventory.add(catalog, id, quantity);
    money.swap(staged_money);
    inventory.swap(staged_inventory);
}

void Shop::sell(std::string_view id, int quantity, Wallet& money,
                Inventory& inventory, const ItemCatalog& catalog) const {
    const auto& entry = offer(id);
    if (catalog.get(id).kind == ItemKind::key_item)
        throw std::invalid_argument("cannot sell key items");
    if (quantity <= 0) throw std::invalid_argument("invalid sale quantity");
    const auto proceeds = static_cast<std::int64_t>(entry.sell_price) * quantity;
    if (proceeds > Wallet::max_money) throw std::invalid_argument("sale overflow");
    Wallet staged_money = money;
    Inventory staged_inventory = inventory;
    staged_inventory.remove(id, quantity);
    if (proceeds != 0) staged_money.credit(static_cast<int>(proceeds));
    money.swap(staged_money);
    inventory.swap(staged_inventory);
}

} // namespace modernps
