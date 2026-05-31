/**
 * @file PatternDatabase.cpp
 * @brief Pattern Database implementation: BFS generation, nibble compression, file I/O
 * @details Supports 48-way symmetry reduction for memory-efficient storage
 */

#include <iostream>
#include <fstream>
#include <queue>
#include <chrono>
#include <cstring>
#include "database/PatternDatabase.h"
#include "database/RankCalculator.h"
#include "database/SymmetryReduction.h"

namespace cube_solver {

PatternDatabase::PatternDatabase(Type type)
    : type(type), maxStates(calculateMaxStates()), generationTime(0) {
}

uint64_t PatternDatabase::calculateMaxStates() const {
    switch (type) {
        case Type::CORNER_PIECES:
            // 8! permutations × 3^7 orientations
            return RankCalculator::factorial(8) * 2187;  // 88,179,840
            
        case Type::EDGE_PERMUTATION:
            // 12! / 2 (parity constraint)
            return RankCalculator::factorial(12) / 2;  // 239,500,800
            
        case Type::EDGE_GROUP_1:
        case Type::EDGE_GROUP_2:
            // P(12,7) / 2 (parity constraint for 7 edges)
            return RankCalculator::permutation(12, 7) / 2;  // 1,995,840
            
        default:
            return 0;
    }
}

uint64_t PatternDatabase::calculateRank(const CubieCube& cube) const {
    switch (type) {
        case Type::CORNER_PIECES:
            return RankCalculator::rankCornerState(cube);
            
        case Type::EDGE_PERMUTATION:
            return RankCalculator::rankEdgePermutation(cube);
            
        case Type::EDGE_GROUP_1: {
            std::array<uint8_t, 7> edges = {0, 1, 2, 3, 4, 5, 6};
            return RankCalculator::rankPartialEdgePermutation(cube, edges);
        }
        
        case Type::EDGE_GROUP_2: {
            std::array<uint8_t, 7> edges = {5, 6, 7, 8, 9, 10, 11};
            return RankCalculator::rankPartialEdgePermutation(cube, edges);
        }
        
        default:
            return 0;
    }
}

uint64_t PatternDatabase::getCanonicalRank(const CubieCube& cube) const {
    if (!useSymmetryReduction || type != Type::CORNER_PIECES) {
        // Only symmetry reduction for corner database currently
        return calculateRank(cube);
    }
    
    // Find canonical form (smallest rank among all 24 symmetries)
    CubieCube canonical = SymmetryReduction::getCanonical(cube);
    return calculateRank(canonical);
}

void PatternDatabase::setDistanceNibble(uint64_t index, uint8_t distance) {
    uint64_t byteIndex = index / 2;
    bool isUpperNibble = (index % 2) == 1;
    
    distance &= 0x0F;  // Ensure 4 bits max
    
    if (byteIndex >= distances.size()) {
        distances.resize(byteIndex + 1, 0xFF);
    }
    
    if (isUpperNibble) {
        distances[byteIndex] = (distances[byteIndex] & 0x0F) | (distance << 4);
    } else {
        distances[byteIndex] = (distances[byteIndex] & 0xF0) | distance;
    }
}

uint8_t PatternDatabase::getDistanceNibble(uint64_t index) const {
    uint64_t byteIndex = index / 2;
    bool isUpperNibble = (index % 2) == 1;
    
    if (byteIndex >= distances.size()) {
        return UNVISITED;
    }
    
    uint8_t byte = distances[byteIndex];
    if (isUpperNibble) {
        return (byte >> 4) & 0x0F;
    } else {
        return byte & 0x0F;
    }
}

uint8_t PatternDatabase::lookup(const CubieCube& cube) const {
    if (distances.empty()) {
        return UNVISITED;
    }
    
    uint64_t rank = useSymmetryReduction ? getCanonicalRank(cube) : calculateRank(cube);
    if (rank >= maxStates) {
        return UNVISITED;
    }
    
    return getDistanceNibble(rank);
}

bool PatternDatabase::generate() {
    std::cout << "Generating pattern database type " << static_cast<int>(type);
    if (useSymmetryReduction) {
        std::cout << " (WITH 48-WAY SYMMETRY REDUCTION)";
    }
    std::cout << "..." << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Initialize distances with all unvisited
    uint64_t numNibbles = maxStates;
    uint64_t numBytes = (numNibbles + 1) / 2;
    distances.resize(numBytes, 0xFF);
    std::cout << "  Allocated " << (numBytes / (1024.0 * 1024.0)) << " MB" << std::endl;
    
    // BFS from solved state
    std::queue<CubieCube> q;
    CubieCube solved;
    uint64_t solvedRank = useSymmetryReduction ? getCanonicalRank(solved) : calculateRank(solved);
    
    setDistanceNibble(solvedRank, 0);
    q.push(solved);
    
    uint64_t statesVisited = 1;
    uint64_t statesExpanded = 0;
    
    while (!q.empty()) {
        CubieCube current = q.front();
        q.pop();
        
        uint64_t currentRank = useSymmetryReduction ? getCanonicalRank(current) : calculateRank(current);
        uint8_t currentDist = getDistanceNibble(currentRank);
        
        if (currentDist >= MAX_DISTANCE) continue;  // Don't expand beyond max
        
        ++statesExpanded;
        if (statesExpanded % 1000000 == 0) {
            std::cout << "  Visited: " << statesVisited << ", Expanded: " << statesExpanded << std::endl;
        }
        
        // Try all 18 moves (6 faces × 3 types)
        for (uint8_t faceIdx = 0; faceIdx < 6; ++faceIdx) {
            for (uint8_t moveTypeIdx = 0; moveTypeIdx < 3; ++moveTypeIdx) {
                CubieCube next = current;
                next.applyMove(
                    static_cast<Face>(faceIdx),
                    static_cast<MoveType>(moveTypeIdx)
                );
                
                uint64_t nextRank = useSymmetryReduction ? getCanonicalRank(next) : calculateRank(next);
                uint8_t nextDist = getDistanceNibble(nextRank);
                
                if (nextDist == UNVISITED) {
                    setDistanceNibble(nextRank, currentDist + 1);
                    q.push(next);
                    ++statesVisited;
                }
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
    generationTime = static_cast<uint32_t>(duration.count());
    
    std::cout << "  Generation complete!" << std::endl;
    std::cout << "  States visited: " << statesVisited << " / " << maxStates << std::endl;
    std::cout << "  Time: " << generationTime << " seconds" << std::endl;
    std::cout << "  Memory: " << (distances.size() / (1024.0 * 1024.0)) << " MB" << std::endl;
    
    return statesVisited > 0;
}

bool PatternDatabase::saveToFile(const std::string& filename) const {
    std::cout << "Saving database to " << filename << "..." << std::endl;
    
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file for writing: " << filename << std::endl;
        return false;
    }
    
    // Create and write header
    PDBHeader header;
    header.version = VERSION;
    header.pdbType = static_cast<uint32_t>(type);
    header.numStates = maxStates;
    header.dataSize = distances.size();
    header.maxDistance = MAX_DISTANCE;
    header.generationTime = generationTime;
    std::memset(header.reserved, 0, sizeof(header.reserved));
    
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!file) {
        std::cerr << "Error: Failed to write header" << std::endl;
        return false;
    }
    
    // Write compressed data
    file.write(reinterpret_cast<const char*>(distances.data()), distances.size());
    if (!file) {
        std::cerr << "Error: Failed to write data" << std::endl;
        return false;
    }
    
    file.close();
    std::cout << "  Saved successfully (" << (distances.size() / (1024.0 * 1024.0)) << " MB)" << std::endl;
    return true;
}

bool PatternDatabase::loadFromFile(const std::string& filename) {
    std::cout << "Loading database from " << filename << "..." << std::endl;
    
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file for reading: " << filename << std::endl;
        return false;
    }
    
    // Read header
    PDBHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file || header.version != VERSION) {
        std::cerr << "Error: Invalid header or version mismatch" << std::endl;
        return false;
    }
    
    if (static_cast<Type>(header.pdbType) != type) {
        std::cerr << "Error: Database type mismatch" << std::endl;
        return false;
    }
    
    // Read compressed data
    distances.resize(header.dataSize);
    file.read(reinterpret_cast<char*>(distances.data()), header.dataSize);
    if (!file) {
        std::cerr << "Error: Failed to read data" << std::endl;
        return false;
    }
    
    file.close();
    generationTime = header.generationTime;
    
    std::cout << "  Loaded successfully (" << (distances.size() / (1024.0 * 1024.0)) << " MB)" << std::endl;
    return true;
}

}  // namespace cube_solver