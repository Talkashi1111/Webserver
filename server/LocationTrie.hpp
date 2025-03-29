#pragma once

#include <string>
#include <map>
#include "Location.hpp"

class LocationTrieNode
{
public:
	LocationTrieNode();
	LocationTrieNode(const LocationTrieNode &other);
	LocationTrieNode &operator=(const LocationTrieNode &other);
	~LocationTrieNode();

	// Children nodes: key is a character, value is a pointer to a child node.
	std::map<char, LocationTrieNode *> children;
	bool isEnd; // True if this node marks the end of a valid location path
	Location *location;

private:
	// Helper function to deep copy children nodes
	void copyChildren(const LocationTrieNode &other);
};

class LocationTrie
{
public:
	LocationTrie();
	// Copy constructor (deep copy of the trie)
	LocationTrie(const LocationTrie &other);
	LocationTrie &operator=(const LocationTrie &other);
	~LocationTrie();

	void insert(Location *loc);

	// Given a URI, return the Location pointer with the longest matching prefix.
	// Returns NULL if no match is found.
	Location *searchLongestPrefix(const std::string &uri) const;
	std::vector<Location *> getAllLocations() const;

private:
	LocationTrieNode *root;

	// Helper function to deep copy a trie node
	static LocationTrieNode *copyNode(const LocationTrieNode *node);
	void collectLocations(const LocationTrieNode *node, std::vector<Location *> &locations) const;
};
