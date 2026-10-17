#pragma once

#include "modernps/items.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace modernps {

// Engine-defined economy; figures are examples, not original ROM prices.
class Wallet {
public:
    static constexpr int max_money = 1'000'000'000;
    explicit Wallet(int money = 0);
    int balance() const noexcept { return money_; }
    void credit(int amount);
    void debit(int amount);
    void swap(Wallet& other) noexcept;
private:
    int money_;
};

struct ShopOffer {
    std::string item_id;
    int buy_price;
    int sell_price;
};

struct ShopDefinition {
    std::string id;
    std::vector<ShopOffer> offers;
};

class Shop {
public:
    explicit Shop(ShopDefinition definition, const ItemCatalog& catalog);
    const ShopDefinition& definition() const noexcept { return definition_; }
    const ShopOffer& offer(std::string_view id) const;
    void buy(std::string_view item_id, int quantity, Wallet& money,
             Inventory& inventory, const ItemCatalog& catalog) const;
    void sell(std::string_view item_id, int quantity, Wallet& money,
              Inventory& inventory, const ItemCatalog& catalog) const;
private:
    ShopDefinition definition_;
    std::map<std::string, ShopOffer, std::less<>> offers_;
};

} // namespace modernps
