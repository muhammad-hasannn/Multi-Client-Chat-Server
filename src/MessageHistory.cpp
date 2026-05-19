#include "../include/MessageHistory.h"

MessageHistory::MessageHistory(int capacity) {
    
    this->capacity = capacity;
    this->top = -1;
    
    // Dynamic array allocation (we created String* in .h file)
    data = new std::string[capacity];
}


MessageHistory::~MessageHistory() {
    delete[] data;
}

bool MessageHistory::isEmpty() const {
    return (top == -1);
}

bool MessageHistory::isFull() const {
    return (top == capacity - 1);
}

/*
Adds a new message to the top of the stack.

If stack becomes full:
oldest messages are shifted left,
newest message inserted at top.
means our 1st message gets lost 
*/

void MessageHistory::push(const std::string& message) {

    if (isFull()) {
        // Shift everything left (the 1st message will get lost)
        for (int i = 0; i < capacity - 1; i++) {
            data[i] = data[i + 1];
        }

        // Insert newest at end
        data[capacity - 1] = message;
        return;
    }
    top++;
    data[top] = message;
}


std::string MessageHistory::pop() {

    if (isEmpty()) return "";

    std::string removedMessage = data[top];
    top--;

    return removedMessage;
}

std::string MessageHistory::peek() const {

    if (isEmpty()) return "";    

    return data[top];
}

int MessageHistory::size() const {
    return top + 1;
}