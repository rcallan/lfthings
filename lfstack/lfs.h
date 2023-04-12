#ifndef ____lfs__
#define ____lfs__

#include <memory>
#include <atomic>
#include <iostream>

/*
note: the CountedNodePtr is a pretty simple struct, so we should be able to use it with the std::atomic<> template for the head of the list.
on platforms that support a double word compare and swap operation, this structure will be small enough for std::atomic<CountedNodePtr>
to be lock free. if this operation isn't available on your platform, it might be better to look for a different strategy for making your stack
lock free

possible alternativve solution: if your system has spare bits in the pointer, eg the address space is only 48 bits but a pointer is 64 bits,
you could store the counter in the spare bits to fit the struct in a single machine word
*/


template <typename T>
class LockFreeStack {
private:
    struct Node;
    struct CountedNodePtr {
        int externalCount;
        Node* ptr {nullptr};
    };
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<int> internalCount;
        CountedNodePtr next;
        Node (T const& data_) : data (std::make_shared<T>(data_)), internalCount (0) { }
    };
    std::atomic<CountedNodePtr> head;
    void increaseHeadCount(CountedNodePtr& oldCounter) {
        CountedNodePtr newCounter;
        do {
            newCounter = oldCounter;
            ++newCounter.externalCount;
        } while (!head.compare_exchange_strong(oldCounter, newCounter, std::memory_order_acquire, std::memory_order_relaxed));
        oldCounter.externalCount = newCounter.externalCount;
    }
public:
    ~LockFreeStack() {
        while(pop() != nullptr);
    }
    void push (T const& data) {
        CountedNodePtr newNode;
        newNode.ptr = new Node(data);
        newNode.externalCount = 1;
        newNode.ptr->next = head.load(std::memory_order_relaxed);
        while (!head.compare_exchange_weak(newNode.ptr->next, newNode, std::memory_order_release, std::memory_order_relaxed));
    }
    std::shared_ptr<T> pop() {
        CountedNodePtr oldHead = head.load(std::memory_order_relaxed);
        while (true) {
            increaseHeadCount(oldHead);
            Node* const ptr = oldHead.ptr;
            if (!ptr) {
                return std::shared_ptr<T>();
            }
            if (head.compare_exchange_strong(oldHead, ptr->next, std::memory_order_relaxed)) {
                std::shared_ptr<T> res;
                res.swap(ptr->data);
                int const countIncrease = oldHead.externalCount - 2;
                if (ptr->internalCount.fetch_add(countIncrease, std::memory_order_release) == -countIncrease) {
                    delete ptr;
                }
                return res;
            } else if (ptr->internalCount.fetch_add(-1, std::memory_order_relaxed) == 1) {
                ptr->internalCount.load(std::memory_order_acquire);
                delete ptr;
            }
        }
    }
};

#endif