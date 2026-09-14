#include "../include/CustomUnorderedMap.h"
#include <iostream>
using namespace std;
CustomUnorderedMap::Node::Node(int key,Order value)
{
    this->key = key;
    this->value = value;
    next = nullptr;
}

CustomUnorderedMap::CustomUnorderedMap(int size)
{
    capacity = size;
    buckets = new Node*[capacity];
    for(int i=0;i<capacity;i++){
        buckets[i] = nullptr;
    }
}
int CustomUnorderedMap::hashFunction(int key) const
{
    return key % capacity;
}
void CustomUnorderedMap::insert(int key,Order value)
{
    int index = hashFunction(key);
    Node* newNode = new Node(key,value);
    // if(buckets[index] == nullptr)
    // {
    //     buckets[index] = newNode;
    //     return;
    // } Not required :)
    // Insert at Head O(1)
    newNode->next = buckets[index];
    buckets[index] = newNode;
}
Order* CustomUnorderedMap::find(int key)
{
    int index = hashFunction(key);
    Node* temp = buckets[index];

    while(temp != nullptr){
        if(temp->key == key)
        {
            return &(temp->value);
        }
        temp = temp->next;
    }
    return nullptr;
}
bool CustomUnorderedMap::remove(int key)
{
    int index = hashFunction(key);

    Node* temp = buckets[index];
    Node* prev = nullptr;

    while(temp!= nullptr){
        if(temp->key == key){
            if(prev == nullptr){
                buckets[index] = temp->next;
            }
            else{
                prev->next = temp->next;
            }
            delete temp;
            return true;
        }
        prev = temp;
        temp = temp->next;
    }
return false;
}
void CustomUnorderedMap::display()
{
    for(int i=0;i<capacity;i++){
        cout<<"Bucket "<<i<<": ";
        Node* temp = buckets[i];
        while(temp != nullptr){
            cout<<"[" << temp->key << "] -> ";
            temp = temp->next;
        }
        cout<<"NULL"<<endl;
    }
}
CustomUnorderedMap::~CustomUnorderedMap()
{
    for(int i=0;i<capacity;i++){
        Node* temp = buckets[i];
        while(temp != nullptr){
            Node* nodeToDelete = temp;
            temp = temp->next;
            delete nodeToDelete;
        }
    }
    delete[] buckets;
}

