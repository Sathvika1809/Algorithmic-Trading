
#include "../include/CustomUnorderedMap.h"
#include <iostream>
#include <stdexcept>

using namespace std;

CustomUnorderedMap::Node::Node(int key, Order value)
{
    this->key = key;
    this->value = value;
    this->next = nullptr;
}

CustomUnorderedMap::CustomUnorderedMap(int size)
{
    if (size <= 0)
    {
        throw invalid_argument("Capacity must be positive");
    }

    capacity = size;
    buckets = new Node*[capacity];

    for (int i = 0; i < capacity; i++)
    {
        buckets[i] = nullptr;
    }
}

int CustomUnorderedMap::hashFunction(int key) const
{
    int index = key % capacity;

    if (index < 0)
    {
        index += capacity;
    }

    return index;
}

void CustomUnorderedMap::insert(int key, Order value)
{
    int index = hashFunction(key);
    Node* current = buckets[index];

    // Update the value if the key already exists.
    while (current != nullptr)
    {
        if (current->key == key)
        {
            current->value = value;
            return;
        }

        current = current->next;
    }

    // Otherwise, insert a new node at the start of the bucket.
    Node* newNode = new Node(key, value);
    newNode->next = buckets[index];
    buckets[index] = newNode;
}

Order* CustomUnorderedMap::find(int key)
{
    int index = hashFunction(key);
    Node* current = buckets[index];

    while (current != nullptr)
    {
        if (current->key == key)
        {
            return &(current->value);
        }

        current = current->next;
    }

    return nullptr;
}

bool CustomUnorderedMap::remove(int key)
{
    int index = hashFunction(key);
    Node* current = buckets[index];
    Node* previous = nullptr;

    while (current != nullptr)
    {
        if (current->key == key)
        {
            if (previous == nullptr)
            {
                buckets[index] = current->next;
            }
            else
            {
                previous->next = current->next;
            }

            delete current;
            return true;
        }

        previous = current;
        current = current->next;
    }

    return false;
}

void CustomUnorderedMap::display()
{
    for (int i = 0; i < capacity; i++)
    {
        cout << "Bucket " << i << ": ";

        Node* current = buckets[i];

        while (current != nullptr)
        {
            cout << "[" << current->key << "] -> ";
            current = current->next;
        }

        cout << "NULL\n";
    }
}

CustomUnorderedMap::~CustomUnorderedMap()
{
    for (int i = 0; i < capacity; i++)
    {
        Node* current = buckets[i];

        while (current != nullptr)
        {
            Node* nodeToDelete = current;
            current = current->next;
            delete nodeToDelete;
        }
    }

    delete[] buckets;
}
