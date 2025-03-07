/*
 *  Serialization and Deserialization of a Doubly-Linked List with Random Pointers
 *
 *  This code demonstrates how to implement serialization and deserialization of a doubly-linked list,
 *  where each node (ListNode) contains data (std::string) and three pointers:
 * - prev: pointer to the previous node in the list,
 * - next: pointer to the next node in the list,
 * - rand: a random pointer to any node in the list or nullptr.
 *
 * - Utilizes modern C++ features (std::unique_ptr, std::make_unique, std::vector,
 *   std::unordered_map) for memory and collection management.
 * - Saves and restores the list in binary format.
 * - Handles I/O errors using exceptions.
 *
 * Eug
 * 2025-03-07
 */

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct ListNode {
  ListNode *prev = nullptr;
  ListNode *next = nullptr;
  ListNode *rand = nullptr;
  std::string data;
};

class List {
public:
  void Serialize(FILE *file); // fopen need for this task
  void Deserialize(FILE *file);

  void AddNode(const std::string &data);
  void SetRand(int nodeIndex, int randIndex);
  int GetCount() const { return count; }
  void Clear();
  void PrintList();
  ~List();

private:
  static uint32_t readUint32(FILE *file);
  static std::unique_ptr<ListNode> readNode(FILE *file, int32_t &outRandIndex);
  static void setupLinks(const std::vector<ListNode *> &nodes);
  static void setupRandPointers(const std::vector<ListNode *> &nodes,
                                const std::vector<int32_t> &randIndices);

  ListNode *head = nullptr;
  ListNode *tail = nullptr;
  int count = 0;
};

void List::AddNode(const std::string &data) {
  ListNode *newNode = new ListNode();
  newNode->data = data;

  if (!head) {
    head = newNode;
    tail = newNode;
  } else {
    tail->next = newNode;
    newNode->prev = tail;
    tail = newNode;
  }

  count++;
}
void List::Serialize(FILE *file) {
  if (!file) {
    throw std::runtime_error("File not open for writing...stopped");
  }

  uint32_t ucount = static_cast<uint32_t>(count);
  if (fwrite(&ucount, sizeof(ucount), 1, file) != 1) {
    throw std::runtime_error("Error writing count...stopped");
  }

  std::vector<ListNode *> nodes;
  ListNode *node = head;
  while (node) {
    nodes.push_back(node);
    node = node->next;
  }

  std::unordered_map<ListNode *, int32_t> nodeToIndex;
  for (size_t i = 0; i < nodes.size(); i++) {
    nodeToIndex[nodes[i]] = static_cast<int32_t>(i);
  }

  for (ListNode *node : nodes) {
    uint32_t dataSize = static_cast<uint32_t>(node->data.size());
    if (fwrite(&dataSize, sizeof(dataSize), 1, file) != 1) {
      throw std::runtime_error("Error writing data size...stopped");
    }

    if (dataSize > 0) {
      if (fwrite(node->data.data(), 1, dataSize, file) != dataSize) {
        throw std::runtime_error("Error writing data...stopped");
      }
    }

    int32_t randIndex = -1;
    if (node->rand != nullptr) {
      randIndex = nodeToIndex[node->rand];
    }
    if (fwrite(&randIndex, sizeof(randIndex), 1, file) != 1) {
      throw std::runtime_error("Error writing rand index...stopped");
    }
  }
}

uint32_t List::readUint32(FILE *file) {
  uint32_t value = 0;
  if (fread(&value, sizeof(value), 1, file) != 1) {
    throw std::runtime_error("Error reading uint32_t value...stopped");
  }
  return value;
}

std::unique_ptr<ListNode> List::readNode(FILE *file, int32_t &outRandIndex) {
  auto node = std::make_unique<ListNode>();
  uint32_t dataSize = readUint32(file);

  if (dataSize > 0) {
    std::string str;
    str.resize(dataSize);
    if (fread(&str[0], 1, dataSize, file) != dataSize) {
      throw std::runtime_error("Error reading node data...stopped");
    }
    node->data = std::move(str);
  }

  if (fread(&outRandIndex, sizeof(outRandIndex), 1, file) != 1) {
    throw std::runtime_error("Error reading rand index...stopped");
  }

  return node;
}

void List::setupLinks(const std::vector<ListNode *> &nodes) {
  size_t n = nodes.size();
  for (size_t i = 0; i < n; i++) {
    if (i > 0) {
      nodes[i]->prev = nodes[i - 1];
    } else {
      nodes[i]->prev = nullptr;
    }

    if (i < n - 1) {
      nodes[i]->next = nodes[i + 1];
    } else {
      nodes[i]->next = nullptr;
    }
  }
}

void List::setupRandPointers(const std::vector<ListNode *> &nodes,
                             const std::vector<int32_t> &randIndices) {
  size_t n = nodes.size();
  for (size_t i = 0; i < n; i++) {
    int32_t randomIndex = randIndices[i];
    if (randomIndex >= 0 && static_cast<size_t>(randomIndex) < n) {
      nodes[i]->rand = nodes[randomIndex];
    } else {
      nodes[i]->rand = nullptr;
    }
  }
}

void List::Deserialize(FILE *file) {
  Clear();

  if (!file) {
    throw std::runtime_error("File not open for reading...stopped");
  }

  uint32_t newCount = readUint32(file);

  std::vector<std::unique_ptr<ListNode>> tempNodes;
  tempNodes.reserve(newCount);
  std::vector<ListNode *> rawNodes;
  rawNodes.reserve(newCount);
  std::vector<int32_t> randIndices;
  randIndices.reserve(newCount);

  for (size_t i = 0; i < newCount; i++) {
    int32_t randomIndex = -1;
    auto node = readNode(file, randomIndex);
    rawNodes.push_back(node.get());
    tempNodes.push_back(std::move(node));
    randIndices.push_back(randomIndex);
  }

  setupLinks(rawNodes);
  setupRandPointers(rawNodes, randIndices);

  if (newCount > 0) {
    head = rawNodes[0];
    tail = rawNodes[newCount - 1];
  } else {
    head = tail = nullptr;
  }
  count = static_cast<int>(newCount);

  for (size_t i = 0; i < newCount; i++) {
    tempNodes[i].release();
  }
}

void List::SetRand(int nodeIndex, int randIndex) {
  if (nodeIndex < 0 || nodeIndex >= count || randIndex < 0 ||
      randIndex >= count) {
    return;
  }

  ListNode *node = head;
  for (size_t i = 0; i < nodeIndex; i++) {
    node = node->next;
  }

  ListNode *randNode = head;
  for (size_t i = 0; i < randIndex; i++) {
    randNode = randNode->next;
  }

  node->rand = randNode;
}

void List::Clear() {
  ListNode *node = head;
  while (node) {
    ListNode *next = node->next;
    delete node;
    node = next;
  }
  head = nullptr;
  tail = nullptr;
  count = 0;
}

List::~List() { Clear(); }

void List::PrintList() {
  ListNode *node = head;
  uint32_t index = 0;
  while (node) {
    std::cout << "Node " << index << ": data = " << node->data << ", rand = ";
    if (node->rand)
      std::cout << node->rand->data;
    else
      std::cout << "nullptr";
    std::cout << std::endl;
    node = node->next;
    ++index;
  }
}

// -------------------- Test Functions --------------------

void TestEmptyList() {
  List list;

  {
    FILE *file = fopen("temp_empty.dat", "wb");
    if (!file) {
      throw std::runtime_error("Can't open file for writing...stopped");
    }
    list.Serialize(file);
    fclose(file);
  }

  List deserialized;
  {
    FILE *file = fopen("temp_empty.dat", "rb");
    if (!file) {
      throw std::runtime_error("Can't open file for reading...stopped");
    }
    deserialized.Deserialize(file);
    fclose(file);
  }
  assert(deserialized.GetCount() == 0);
  std::cout << "TestEmptyList passed" << std::endl;
}

void TestSingleNode() {
  List list;
  list.AddNode("SingleNode");
  list.SetRand(0, 0); // self-reference
  {
    FILE *file = fopen("temp_single.dat", "wb");
    if (!file) {
      throw std::runtime_error("Can't open file for writing");
    }
    list.Serialize(file);
    fclose(file);
  }
  List deserialized;
  {
    FILE *file = fopen("temp_single.dat", "rb");
    if (!file) {
      throw std::runtime_error("Can't open file for reading");
    }
    deserialized.Deserialize(file);
    fclose(file);
  }
  assert(deserialized.GetCount() == 1);
  std::cout << "TestSingleNode:" << std::endl;
  deserialized.PrintList();
  std::cout << "TestSingleNode passed" << std::endl;
}

void TestMultipleNodes() {
  List list;
  list.AddNode("Node1");
  list.AddNode("Node2");
  list.AddNode("Node3");
  list.AddNode("Node4");
  list.AddNode("Node5");
  list.SetRand(0, 2);
  list.SetRand(1, 4);
  list.SetRand(2, 0);
  list.SetRand(3, 3);
  list.SetRand(4, 1);

  {
    FILE *file = fopen("temp_multiple.dat", "wb");
    if (!file) {
      throw std::runtime_error("Can't open file for writing");
    }
    list.Serialize(file);
    fclose(file);
  }
  List deserialized;
  {
    FILE *file = fopen("temp_multiple.dat", "rb");
    if (!file) {
      throw std::runtime_error("Can't open file for reading...stopped");
    }
    deserialized.Deserialize(file);
    fclose(file);
  }
  assert(deserialized.GetCount() == 5);
  std::cout << "TestMultipleNodes:" << std::endl;
  deserialized.PrintList();
  std::cout << "TestMultipleNodes passed" << std::endl;
}

// -------------------- Main Function --------------------

int main() {
  try {
    std::cout << "Running tests..." << std::endl;
    TestEmptyList();
    TestSingleNode();
    TestMultipleNodes();
  } catch (const std::exception &ex) {
    std::cerr << "Test failed: " << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
