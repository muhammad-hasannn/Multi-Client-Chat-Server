#include <string>

// A fixed-size Stack of the last N broadcast messages.
// New clients receive these on join (replay feature).

class MessageHistory {
public:
    explicit MessageHistory(int capacity = 20);
    ~MessageHistory();

    void push(const std::string& message);
    std::string pop();
    std::string peek() const;
    bool isEmpty() const;
    bool isFull() const;
    int size() const;

private:
    std::string* data;
    int top;
    int capacity;
};