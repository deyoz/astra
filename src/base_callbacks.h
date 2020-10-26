#ifndef BASE_CALLBACKS_H
#define BASE_CALLBACKS_H

#include "serverlib/logger.h"
#include <stdexcept>
#include <memory>
#include <vector>

template <class T>
class CallbacksSingleton
{
    private:
        std::shared_ptr<T> m_cb;

    protected:
        CallbacksSingleton()
        {
            m_cb = nullptr;
        }

    public:
        static CallbacksSingleton* Instance()
        {
            static CallbacksSingleton* inst = nullptr;
            if(!inst) {
                inst = new CallbacksSingleton;
            }
            return inst;
        }
        const std::shared_ptr<T> & getCallbacks()
        {
            if(m_cb) {
                return m_cb;
            }
            throw std::logic_error("getCallbacks: not initialized");
        }
        void setCallbacks(T* cb)
        {
            m_cb.reset(cb);
        }
};

template <class T>
inline const std::shared_ptr<T> & callbacks()
{
    return CallbacksSingleton<T>::Instance()->getCallbacks();
}

template <class T>
class CallbacksSuite
{
    private:
        std::vector<std::shared_ptr<T>> callbacks;

    protected:
        CallbacksSuite()
        {
        }

    public:
        static CallbacksSuite* Instance()
        {
            static CallbacksSuite* inst = nullptr;
            if(!inst) {
                inst = new CallbacksSuite;
            }
            return inst;
        }
        const std::vector<std::shared_ptr<T>> & getCallbacks() const
        {
            if(!callbacks.empty()) {
                return callbacks;
            }
            throw std::logic_error("getCallbacks: callbacks empty!");
        }
        void addCallbacks(T* cb)
        {
            callbacks.push_back(std::shared_ptr<T>(cb));
        }
};


template <class T>
inline const std::vector<std::shared_ptr<T>> & callbacksSuite()
{
    return CallbacksSuite<T>::Instance()->getCallbacks();
}

void CallbacksExceptionFilter(STDLOG_SIGNATURE);
void CallbacksExceptionFilter(TRACE_SIGNATURE);

#endif // BASE_CALLBACKS_H
