#include "LocationTrie.hpp"

// ----------------------
// LocationTrieNode methods
// ----------------------

LocationTrieNode::LocationTrieNode() : isEnd(false), location(NULL) {}

// Helper: deep copy children from another node.
void LocationTrieNode::copyChildren(const LocationTrieNode &other)
{
	for (std::map<char, LocationTrieNode *>::const_iterator it = other.children.begin();
		 it != other.children.end(); ++it)
	{
		// Deep copy each child node
		children[it->first] = new LocationTrieNode(*(it->second));
	}
}

LocationTrieNode::LocationTrieNode(const LocationTrieNode &other)
	: isEnd(other.isEnd), location(other.location) // shallow copy of location pointer
{
	copyChildren(other);
}

LocationTrieNode &LocationTrieNode::operator=(const LocationTrieNode &other)
{
	if (this != &other)
	{
		// First, delete current children
		for (std::map<char, LocationTrieNode *>::iterator it = children.begin();
			 it != children.end(); ++it)
		{
			delete it->second;
		}
		children.clear();

		isEnd = other.isEnd;
		location = other.location; // shallow copy

		copyChildren(other);
	}
	return *this;
}

LocationTrieNode::~LocationTrieNode()
{
	for (std::map<char, LocationTrieNode *>::iterator it = children.begin();
		 it != children.end(); ++it)
	{
		delete it->second;
	}
	children.clear();
}

// ----------------------
// LocationTrie methods
// ----------------------

LocationTrie::LocationTrie()
{
	root = new LocationTrieNode();
}

// Helper: deep copy a node recursively.
LocationTrieNode *LocationTrie::copyNode(const LocationTrieNode *node)
{
	if (!node)
		return NULL;
	LocationTrieNode *newNode = new LocationTrieNode(*node);
	return newNode;
}

LocationTrie::LocationTrie(const LocationTrie &other)
{
	root = copyNode(other.root);
}

LocationTrie &LocationTrie::operator=(const LocationTrie &other)
{
	if (this != &other)
	{
		delete root;
		root = copyNode(other.root);
	}
	return *this;
}

LocationTrie::~LocationTrie()
{
	delete root;
}

// Insert a Location pointer into the trie using its URI path.
void LocationTrie::insert(Location *loc)
{
	if (!loc)
		return;
	std::string path = loc->getPath();
	LocationTrieNode *node = root;
	for (size_t i = 0; i < path.size(); ++i)
	{
		char c = path[i];
		if (node->children.find(c) == node->children.end())
		{
			node->children[c] = new LocationTrieNode();
		}
		node = node->children[c];
	}
	node->isEnd = true;
	node->location = loc;
}

// Given a URI, traverse the trie to find the longest matching location prefix.
// Returns the pointer to the Location if found; otherwise, returns NULL.
Location *LocationTrie::searchLongestPrefix(const std::string &uri) const
{
	LocationTrieNode *node = root;
	Location *lastFound = NULL;
	for (size_t i = 0; i < uri.size(); ++i)
	{
		char c = uri[i];
		if (node->children.find(c) == node->children.end())
			break;
		node = node->children[c];
		if (node->isEnd)
			lastFound = node->location;
	}
	return lastFound;
}

void LocationTrie::collectLocations(const LocationTrieNode *node, std::vector<Location *> &locations) const
{
	if (!node)
		return;
	if (node->isEnd && node->location)
		locations.push_back(node->location);
	for (std::map<char, LocationTrieNode *>::const_iterator it = node->children.begin();
		 it != node->children.end(); ++it)
	{
		collectLocations(it->second, locations);
	}
}

std::vector<Location *> LocationTrie::getAllLocations() const
{
	std::vector<Location *> locations;
	collectLocations(root, locations);
	return locations;
}
