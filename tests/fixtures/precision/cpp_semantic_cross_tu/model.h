#pragma once

struct Base {
    virtual int run();
};

struct Derived : Base {
    int run() override;
};

template <typename T>
struct Box {};

int callBase(Base& base);
Box<int>* makeBox();
