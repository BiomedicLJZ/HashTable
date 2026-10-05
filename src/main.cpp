#include <iostream>
#include <string>

#include "HashTable.hpp"

void demo(CollisionResolution method, const std::string& label) {
    std::cout << "=== " << label << " ===\n";

    HashTable<std::string, int> table(method, 7);

    table.insert("uno", 1);
    table.insert("dos", 2);
    table.insert("tres", 3);
    table.insert("cuatro", 4);
    table.insert("cinco", 5);
    table.insert("seis", 6);
    table.insert("siete", 7);
    table.insert("ocho", 8);

    table.remove("tres");
    table.insert("nueve", 9);

    std::cout << "size: " << table.size() << ", capacity: " << table.capacity()
              << ", loadFactor: " << table.loadFactor() << "\n";

    for (const auto& [key, value] : table.items()) {
        std::cout << key << " -> " << value << "\n";
    }

    std::cout << "contains(\"tres\"): " << std::boolalpha << table.contains("tres") << "\n";
    std::cout << "contains(\"nueve\"): " << std::boolalpha << table.contains("nueve") << "\n";
    std::cout << "\n";
}

int main() {
    demo(CollisionResolution::CHAINING, "Encadenamiento separado");
    demo(CollisionResolution::LINEAR_PROBING, "Sondeo lineal");
    demo(CollisionResolution::QUADRATIC_PROBING, "Sondeo cuadratico");
    demo(CollisionResolution::DOUBLE_HASHING, "Doble hashing");

    return 0;
}
