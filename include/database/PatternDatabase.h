#ifndef PATTERN_DATABASE_H
#define PATTERN_DATABASE_H

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <array>
#include <chrono>

/**
 * @file PatternDatabase.h
 * @brief Pattern Database (PDB) implementation for Korf's IDA* solver
 * * Stores BFS-computed distances for cube subsets:
 * - Corner PDB: All 8 corner positions + orientations (88.2M states) → 42 MB
 * - Edge Permutation PDB: 12-edge permutations with parity (239.5M states) → 114 MB  
 * - 7-Edge PDBs (2x): 7-edge subsets (2M states each) → 1 MB each
 * * Storage uses nibble compression: 2 distances per byte (4 bits each)
 * Generation uses BFS from solved state (10-90 seconds per database)
 */

#include "cube/CubieCube.h"
#include "database/RankCalculator.h"

namespace cube_solver {

/**
 * @class PatternDatabase
 * @brief Concrete pattern database implementation
 */
class PatternDatabase {
public:
    enum class Type : uint32_t {
        CORNER_PIECES = 0,      // 8 corners permutation + orientation
        EDGE_PERMUTATION = 1,   // 12 edges permutation (even parity)
        EDGE_GROUP_1 = 2,       // First 7 edges permutation
        EDGE_GROUP_2 = 3        // Remaining 5 edges permutation
    };
    
    /**
     * @brief Construct pattern database of specified type
     * @param type Which subset to track (corners, edges, etc.)
     */
    explicit PatternDatabase(Type type);
    
    /**
     * @brief Look up distance from cube state to solved state
     * @param cube State to query
     * @return Distance estimate (0-11 moves, or 15 if not reached)
     */
    uint8_t lookup(const CubieCube& cube) const;
    
    /**
     * @brief Generate entire database using BFS
     * @return true if successful, false if already exists
     * * Algorithm:
     * 1. Initialize solved state with distance 0
     * 2. BFS: expand each state via 18 moves (6 faces × 3 types)
     * 3. Mark unvisited successors with distance = current + 1
     * 4. Stop when all reachable states visited or distance limit reached
     * 5. Store in nibble-compressed format
     */
    bool generate();
    
    /**
     * @brief Save database to binary file with header
     * @param filename Path to save to
     * @return true if successful
     */
    bool saveToFile(const std::string& filename) const;
    
    /**
     * @brief Load database from binary file
     * @param filename Path to load from
     * @return true if successful
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * @brief Get EXACT physical memory footprint of database in bytes
     * Queries the underlying vector capacity for real-world RAM usage.
     */
    size_t getMemoryFootprint() const {
        return distances.capacity() * sizeof(uint8_t);
    }
    
    /**
     * @brief Check if database is valid and loaded
     */
    bool isValid() const { return !distances.empty(); }
    
    /**
     * @brief Get the type of this database
     */
    Type getType() const { return type; }
    
    /**
     * @brief Get generation time in seconds (if loaded from file)
     */
    uint32_t getGenerationTime() const { return generationTime; }
    
    /**
     * @brief Enable/disable 48-way symmetry reduction
     * * When enabled, only stores one representative per symmetry class,
     * reducing memory by ~48× (88.2M states → 1.8M states).
     * * During lookup, queries are automatically converted to canonical form.
     * * @param enabled true to use symmetry reduction, false for full database
     */
    void setSymmetryReduction(bool enabled) { useSymmetryReduction = enabled; }
    
    /**
     * @brief Check if symmetry reduction is enabled
     */
    bool isSymmetryReductionEnabled() const { return useSymmetryReduction; }
    
private:
    static constexpr uint8_t UNVISITED = 0xFF;
    static constexpr uint8_t MAX_DISTANCE = 11;
    static constexpr uint32_t VERSION = 1;
    
    struct PDBHeader {
        uint32_t version;        // Must be 1
        uint32_t pdbType;        // Type enum value
        uint64_t numStates;      // Total states covered
        uint64_t dataSize;       // Bytes of compressed data
        uint32_t maxDistance;    // Max distance stored
        uint32_t generationTime; // Seconds to generate
        char reserved[40];       // Future use
    } __attribute__((packed));  // Ensure no padding
    
    Type type;
    std::vector<uint8_t> distances;  // Nibble-compressed (2 per byte)
    uint64_t maxStates;
    uint32_t generationTime;
    bool useSymmetryReduction = false;  // Enable 48-way symmetry reduction
    
    // Helper functions
    uint64_t calculateRank(const CubieCube& cube) const;
    uint64_t getCanonicalRank(const CubieCube& cube) const;  // With symmetry reduction
    void setDistanceNibble(uint64_t index, uint8_t distance);
    uint8_t getDistanceNibble(uint64_t index) const;
    uint64_t calculateMaxStates() const;
};

}  // namespace cube_solver

#endif  // PATTERN_DATABASE_H



//---------------------------------------
// #ifndef PATTERN_DATABASE_H

// #define PATTERN_DATABASE_H



// #include <cstdint>

// #include <memory>

// #include <vector>

// #include <string>

// #include <array>

// #include <chrono>



// /**

//  * @file PatternDatabase.h

//  * @brief Pattern Database (PDB) implementation for Korf's IDA* solver

//  * 

//  * Stores BFS-computed distances for cube subsets:

//  * - Corner PDB: All 8 corner positions + orientations (88.2M states) → 42 MB

//  * - Edge Permutation PDB: 12-edge permutations with parity (239.5M states) → 114 MB  

//  * - 7-Edge PDBs (2x): 7-edge subsets (2M states each) → 1 MB each

//  * 

//  * Storage uses nibble compression: 2 distances per byte (4 bits each)

//  * Generation uses BFS from solved state (10-90 seconds per database)

//  */



// #include "cube/CubieCube.h"

// #include "database/RankCalculator.h"



// namespace cube_solver {



// /**

//  * @class PatternDatabase

//  * @brief Concrete pattern database implementation

//  */

// class PatternDatabase {

// public:

//     enum class Type : uint32_t {

//         CORNER_PIECES = 0,      // 8 corners permutation + orientation

//         EDGE_PERMUTATION = 1,   // 12 edges permutation (even parity)

//         EDGE_GROUP_1 = 2,       // First 7 edges permutation

//         EDGE_GROUP_2 = 3        // Remaining 5 edges permutation

//     };

    

//     /**

//      * @brief Construct pattern database of specified type

//      * @param type Which subset to track (corners, edges, etc.)

//      */

//     explicit PatternDatabase(Type type);

    

//     /**

//      * @brief Look up distance from cube state to solved state

//      * @param cube State to query

//      * @return Distance estimate (0-11 moves, or 15 if not reached)

//      */

//     uint8_t lookup(const CubieCube& cube) const;

    

//     /**

//      * @brief Generate entire database using BFS

//      * @return true if successful, false if already exists

//      * 

//      * Algorithm:

//      * 1. Initialize solved state with distance 0

//      * 2. BFS: expand each state via 18 moves (6 faces × 3 types)

//      * 3. Mark unvisited successors with distance = current + 1

//      * 4. Stop when all reachable states visited or distance limit reached

//      * 5. Store in nibble-compressed format

//      */

//     bool generate();

    

//     /**

//      * @brief Save database to binary file with header

//      * @param filename Path to save to

//      * @return true if successful

//      */

//     bool saveToFile(const std::string& filename) const;

    

//     /**

//      * @brief Load database from binary file

//      * @param filename Path to load from

//      * @return true if successful

//      */

//     bool loadFromFile(const std::string& filename);

    

//     /**

//      * @brief Get memory usage of database in bytes

//      */

//     uint64_t getMemoryUsage() const;

    

//     /**

//      * @brief Check if database is valid and loaded

//      */

//     bool isValid() const { return !distances.empty(); }

    

//     /**

//      * @brief Get the type of this database

//      */

//     Type getType() const { return type; }

    

//     /**

//      * @brief Get generation time in seconds (if loaded from file)

//      */

//     uint32_t getGenerationTime() const { return generationTime; }

    

//     /**

//      * @brief Enable/disable 48-way symmetry reduction

//      * 

//      * When enabled, only stores one representative per symmetry class,

//      * reducing memory by ~48× (88.2M states → 1.8M states).

//      * 

//      * During lookup, queries are automatically converted to canonical form.

//      * 

//      * @param enabled true to use symmetry reduction, false for full database

//      */

//     void setSymmetryReduction(bool enabled) { useSymmetryReduction = enabled; }

    

//     /**

//      * @brief Check if symmetry reduction is enabled

//      */

//     bool isSymmetryReductionEnabled() const { return useSymmetryReduction; }

    

// private:

//     static constexpr uint8_t UNVISITED = 0xFF;

//     static constexpr uint8_t MAX_DISTANCE = 11;

//     static constexpr uint32_t VERSION = 1;

    

//     struct PDBHeader {

//         uint32_t version;        // Must be 1

//         uint32_t pdbType;        // Type enum value

//         uint64_t numStates;      // Total states covered

//         uint64_t dataSize;       // Bytes of compressed data

//         uint32_t maxDistance;    // Max distance stored

//         uint32_t generationTime; // Seconds to generate

//         char reserved[40];       // Future use

//     } __attribute__((packed));  // Ensure no padding

    

//     Type type;

//     std::vector<uint8_t> distances;  // Nibble-compressed (2 per byte)

//     uint64_t maxStates;

//     uint32_t generationTime;

//     bool useSymmetryReduction = false;  // Enable 48-way symmetry reduction

    

//     // Helper functions

//     uint64_t calculateRank(const CubieCube& cube) const;

//     uint64_t getCanonicalRank(const CubieCube& cube) const;  // With symmetry reduction

//     void setDistanceNibble(uint64_t index, uint8_t distance);

//     uint8_t getDistanceNibble(uint64_t index) const;

//     uint64_t calculateMaxStates() const;

// };



// }  // namespace cube_solver



// #endif  // PATTERN_DATABASE_H

