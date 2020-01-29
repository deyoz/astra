#ifndef ACCESSIBLE_H
#define ACCESSIBLE_H

class Accessible
{
public:
    virtual ~Accessible();
    virtual bool state() const = 0;
};

class IAccessible
{
protected:
    IAccessible(Accessible* acc = NULL);
public:
    virtual ~IAccessible();
    bool isAccessible() const;
private:
    Accessible* acc_;
};



#endif /* ACCESSIBLE_H */

