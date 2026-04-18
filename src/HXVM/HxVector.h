#pragma once
#include <iostream>
#include <cstdlib>
#include <wchar.h>
#include <unistd.h>
#include <cstdio>
#include <locale.h>
#include <stdint.h>
#include <cstring>

//#define HXVECTOR_DEBUG
#define HXVECTOR_LOG_DEB_LABEL L"\33[33;1m[DEB]\33[0m"
#define HXVECTOR_LOG_ERR_LABEL L"\33[31;1m[ERR]\33[0m"

template <typename T>
class HxVector {
private:
    T* data;
    uint32_t top;
    uint32_t dataCapacity;
public:
    typedef struct Element {  //包装着状态的元素，作为at()的返回类型
        bool isOutOfArray;
        T* value;
    } Element;
#ifdef HXVECTOR_DEBUG
    FILE* logStream;
#endif
    FILE* errStream;
    int retryDelay;  //出错重试时停留的时间

    HxVector(const HxVector& other) noexcept {
        // 基础数据拷贝
        this->dataCapacity = other.dataCapacity;
        this->top = other.top;
        this->retryDelay = other.retryDelay;

        this->errStream = stdout;
#ifdef HXVECTOR_DEBUG
        this->logStream = stdout;
#endif

        if (other.data) {
            this->data = new(std::nothrow) T[this->dataCapacity];
            if (this->data) {
                std::memcpy(this->data, other.data, this->top * sizeof(T));
            }
        } else {
            this->data = nullptr;
        }
    }
    HxVector& operator=(const HxVector& other) noexcept {
        if (this != &other) {
            // 容量不够就重新申请喵
            if (this->dataCapacity < other.top) {
                if (this->data != nullptr) {
                    delete[] this->data;
                }
                this->dataCapacity = other.dataCapacity;
                this->data = new(std::nothrow) T[this->dataCapacity];
            }

            // 覆盖数据喵
            this->top = other.top;
            this->retryDelay = other.retryDelay;
            this->errStream = other.errStream;
#ifdef HXVECTOR_DEBUG
            this->logStream = other.logStream;
#endif
            for (uint32_t i = 0; i < this->top; i++) {
                this->data[i] = other.data[i];
            }
        }
        return *this;
    }
    HxVector& operator=(HxVector&& other) noexcept {
        if (this != &other) {
            if (this->data) delete[] this->data; // 释放自己的旧资源
            this->data = other.data;             // 抢走别人的资源
            this->top = other.top;
            this->dataCapacity = other.dataCapacity;
            other.data = nullptr;                // 把别人变成空壳
            other.top = 0;
            other.dataCapacity = 0;
        }
        return *this;
    }
    HxVector() noexcept {
        this-> retryDelay = 100;
        this-> errStream = stdout;
#ifdef HXVECTOR_DEBUG
        this-> logStream = stdout;
#endif
        this-> dataCapacity = 0;
        this-> top = 0;
        this-> data = nullptr;
#ifdef HXVECTOR_DEBUG
        fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"初始化\n");
#endif
        return;
    }
    HxVector(uint32_t initSize) noexcept {
        setlocale(LC_ALL, "C.UTF-8");
        this-> retryDelay = 100;
        this-> dataCapacity = 0;
        this-> top = 0;
        this-> errStream = stdout;
#ifdef HXVECTOR_DEBUG
        this-> logStream = stdout;
#endif
#ifdef HXVECTOR_DEBUG
        fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"初始化\n");
#endif
        this->data = new(std::nothrow) T[initSize];
        while(this-> data == NULL || this-> data == nullptr) {
            usleep(retryDelay);
            fwprintf(this-> errStream, HXVECTOR_LOG_ERR_LABEL L"分配失败，重试中...\n");
            this->data = new(std::nothrow) T[initSize];
        }

        this-> dataCapacity = initSize;
#ifdef HXVECTOR_DEBUG
        fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"分配成功\n");
#endif
    }
    void push_back(T&& element) noexcept {
        if (this->top >= this->dataCapacity) {
            this->reserve(this->dataCapacity == 0 ? 2 : this->dataCapacity * 2);
        }
        this->data[this->top] = std::move(element);
        this->top++;
    }
    void push_back(T element) noexcept {
        //扩容
        if(this-> top >= this-> dataCapacity) {
            int32_t oldCapacity = this->dataCapacity;
            this->dataCapacity = (oldCapacity == 0) ? 2 : oldCapacity * 2;
            T* newData = new(std::nothrow) T[this-> dataCapacity];
            while(newData == nullptr || newData == NULL) {
                usleep(retryDelay);
                fwprintf(this-> errStream, HXVECTOR_LOG_ERR_LABEL L"分配失败，重试中...\n");
                if(this-> dataCapacity > oldCapacity+1) this->dataCapacity--;
                newData = new(std::nothrow) T[this-> dataCapacity];
            }

            if (this->data != nullptr) {
                for (uint32_t i = 0; i < this->top; i++) {
                    newData[i] = std::move(this->data[i]);
                }
                delete[] this->data;
            }
            this->data = newData;
#ifdef HXVECTOR_DEBUG
            fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"分配成功\n");
#endif
        }
        this-> data[this-> top] = element;
        this-> top++;
#ifdef HXVECTOR_DEBUG
        fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"已压入一个元素，栈顶指针指向第%u槽位，当前容量为 %u\n", this-> top, this-> dataCapacity);
#endif
        return;
    }
    void pop_back(void) noexcept {
        this-> top--;
        this-> shrink_to_fit();
    }
    typename HxVector<T>::Element at(uint32_t index) noexcept {
        typename HxVector<T>::Element element = {};
        element.isOutOfArray = false;
        element.value = nullptr;
        if(index >= this-> top) {
            fwprintf(this->errStream, HXVECTOR_LOG_ERR_LABEL L"越界！\n");
            element.isOutOfArray = true;
            return element;
        }
        element.isOutOfArray = false;
        element.value = this->data[index];
        return element;
    }
    //不安全
    T& operator[](uint32_t index) noexcept {
#ifdef HXVECTOR_DEBUG
        fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"下标访问 %u\n", index);
#endif
        if(index >= this-> top) {
            fwprintf(this->errStream, HXVECTOR_LOG_ERR_LABEL L"越界！\n");
            fflush(this->errStream);
            __builtin_trap();
        }
        return std::ref(this->data[index]);
    }
    const T& operator[](uint32_t index) const noexcept {
#ifdef HXVECTOR_DEBUG
        fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"下标访问 %u\n", index);
#endif
        if(index >= this-> top) {
            fwprintf(this->errStream, HXVECTOR_LOG_ERR_LABEL L"越界！\n");
            fflush(this->errStream);
            __builtin_trap();
        }
        return std::ref(this->data[index]);
    }
    ~HxVector() noexcept {
#ifdef HXVECTOR_DEBUG
        if (this->logStream != nullptr) fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"释放\n");
#endif
        if(this->data != nullptr && this->data != NULL) {
            delete[] this->data;
            this->data = nullptr;
        }
#ifdef HXVECTOR_DEBUG
        if (this->logStream != nullptr) fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"所有元素已释放\n");
#endif
    }
    uint32_t size() noexcept {
        return this->top;
    }
    uint32_t capacity() noexcept {
        return this-> dataCapacity;
    }
    bool empty() {
        if(this-> top == 0) return true;
        else return false;
    }
    //缩容
    void shrink_to_fit() noexcept {
        if(this->top == 0) return;
        T* newData = new(std::nothrow) T[this->top];
        if(newData == nullptr || newData == NULL) {
#ifdef HXVECTOR_DEBUG
            fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"算了不缩容了\n");
#endif
            return;
        }
        for(uint32_t i = 0; i < this-> top; i++) {
            newData[i] = std::move(this->data[i]);
        }
        delete[] this->data;
        this-> data = newData;
        this-> dataCapacity = this->top;
#ifdef HXVECTOR_DEBUG
        fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"缩容至 %u\n", this-> dataCapacity);
#endif
    }
    //只分配槽位，top不动
    void reserve(uint32_t newCapacity) noexcept {
        if(newCapacity <= this->dataCapacity) return;
#ifdef HXVECTOR_DEBUG
        fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"槽位扩容至%u\n", newCapacity);
#endif
        uint32_t oldCapacity = this->dataCapacity;
        this->dataCapacity = newCapacity;
        T* newData = new(std::nothrow) T[this-> dataCapacity];
        while(newData == nullptr || newData == NULL) {
            usleep(retryDelay);
            fwprintf(this-> errStream, HXVECTOR_LOG_ERR_LABEL L"分配失败，重试中...\n");
            newData = new(std::nothrow) T[this-> dataCapacity];
        }
        if (this->data != nullptr) {
            for (uint32_t i = 0; i < this->top; i++) {
                newData[i] = std::move(this->data[i]);
            }
            delete[] this->data;
        }
        this->data = newData;

#ifdef HXVECTOR_DEBUG
        fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"分配成功\n");
#endif
        return;
    }
    //扩容，top移动
    void resize(uint32_t newSize) noexcept {
        if(newSize <= this->top) return;
#ifdef HXVECTOR_DEBUG
        fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"扩容至%u\n", newSize);
#endif
        this->reserve(newSize);
#ifdef HXVECTOR_DEBUG
        fwprintf(this-> logStream, HXVECTOR_LOG_DEB_LABEL L"分配成功\n");
#endif
        this->top = newSize;
    }
};