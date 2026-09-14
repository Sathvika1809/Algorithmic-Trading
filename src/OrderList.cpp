#include "../include/OrderList.h"
#include <iostream>

using namespace std;

OrderList::Node::Node(Order o)
{
    order = o;
    next = nullptr;
}
OrderList::OrderList()
{
    // head is NULL the first list is empty
    head = nullptr;
}
void OrderList::insert(Order order)
{
    Node* newNode = new Node(order);
    if(head == nullptr)
    {
        head = newNode;
        return;
    }
    Node* temp = head;

    while(temp->next != nullptr){
        temp = temp->next;
    }
    temp->next = newNode;
}
Order* OrderList::find(int orderId)
{
    Node* temp = head;
    while(temp != nullptr)
    {
        if(temp->order.getOrderId() == orderId)
        {
            return &(temp->order);
        }
        temp = temp->next;
    }

    return nullptr;
}

bool OrderList::remove(int orderId)
{
    if(head == nullptr)
    {
        return false;
    }
    //If the first node has the required order
    if(head->order.getOrderId() == orderId)
    {
        Node* temp = head;
        head = head->next;

        delete temp;
        return true;
    }
    Node* temp = head;
    while(temp->next != nullptr)
    {
        if(temp->next->order.getOrderId() == orderId)
        {
            Node* nodeToDelete = temp->next;
            temp->next = temp->next->next;
            delete nodeToDelete;

            return true;
        }
        temp = temp->next;
    }
    return false;
}
void OrderList::display()
{
    Node* temp = head;
    while(temp != nullptr){
        temp->order.display();
        cout<<"-----------"<<endl;
        temp = temp->next;
    }
}