#pragma once

namespace balancer {

class Lock
{
    enum class State { UNLOCKED = 0, SHARED, EXCLUSIVE };

public:
    explicit Lock(const std::string& name);
    ~Lock();

    Lock(const Lock& ) = delete;
    Lock& operator=(const Lock& ) = delete;

    Lock(Lock&& ) = delete;
    Lock& operator=(Lock&& ) = delete;

    bool try_to_unique_lock();
    void unique_lock();
    void shared_lock();

    void unlock();

private:
    int fd_;
    State state_;
    const std::string name_;
};

class Barrier
{
public:
    explicit Barrier(const int key);
    ~Barrier();

    Barrier(const Barrier& ) = delete;
    Barrier& operator=(const Barrier& ) = delete;

    Barrier(Barrier&& ) = delete;
    Barrier& operator=(Barrier&& ) = delete;

    int value(const unsigned short num) const;

    bool post(const unsigned short num);

    bool wait(const unsigned short num);

    bool wait_all(const unsigned short num);

private:
    const int id_;
};

} // namespace balancer
