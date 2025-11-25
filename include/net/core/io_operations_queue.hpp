#ifndef __VJIK_NET_CORE_IO_OPS_QUEUE_HPP
#define __VJIK_NET_CORE_IO_OPS_QUEUE_HPP

#include <cstring>
#include <netdb.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>
#include <unordered_map>

namespace vjik {
namespace net {
namespace core {

template <typename Descriptor> 
class io_operations_queue {
public:
  io_operations_queue() 
    : operations_(),
      cleanup_operations_(nullptr) {};

  template <typename Handler>
  auto enqueue_op(Descriptor desc, Handler handler) -> bool {
    op_base *new_op = new op(desc, handler); 
    auto result = operations_.insert({ desc, new_op });
    if (result.second) return true;

    op_base *curr = result.first->second; 
    while (curr->next_) {
      curr = curr->next_;
    }
    curr->next_ = new_op;
    return false;
  }

  template <typename Descriptor_Set>
  auto get_descriptors(Descriptor_Set& descriptors)
  {
    typename operation_map::iterator i = operations_.begin();
    while (i != operations_.end())
    {
      descriptors.set(i->first);
      ++i;
    }
  }

  template <typename Descriptor_Set>
  auto dispatch_io_operations(const Descriptor_Set& descriptors) {
    auto i = operations_.begin();
    while (i != operations_.end())
    {
      typename operation_map::iterator op_iter = i++;
      if (descriptors.is_set(op_iter->first))
      {
        op_base* this_op = op_iter->second;
        op_iter->second = this_op->next_;
        this_op->next_ = cleanup_operations_;
        cleanup_operations_ = this_op;
        bool done = this_op->invoke();
        if (done) {
          if (!op_iter->second)
            operations_.erase(op_iter);
        } else {
          cleanup_operations_ = this_op->next_;
          this_op->next_ = op_iter->second;
          op_iter->second = this_op;
      }
      }
    }
  }

  auto cleanup_operations() {
    while (cleanup_operations_) {
      op_base* next_op = cleanup_operations_->next_;
      cleanup_operations_->next_ = 0;
      cleanup_operations_->destroy();
      cleanup_operations_ = next_op;
    }
  }


private:
  class op_base {
  public:
    auto invoke() -> bool { 
      return invoke_func_(this);
    }

    void destroy() { 
      return destroy_func_(this);
    }

  protected:
    typedef bool (*invoke_func_type)(op_base *);
    typedef void (*destroy_func_type)(op_base *);
    op_base(Descriptor desc, invoke_func_type invoke_func,
            destroy_func_type destroy_func) 
    : desc_(desc),
      invoke_func_(invoke_func),
      destroy_func_(destroy_func),
      next_(nullptr) {}

    ~op_base() {}

  private:
    friend class io_operations_queue<Descriptor>;

    Descriptor desc_;
    invoke_func_type invoke_func_;
    destroy_func_type destroy_func_;
    op_base *next_;
  };

  template <typename Handler> 
  class op : public op_base {
  public:
    op(Descriptor desc, Handler handler) 
      : op_base(desc, &op<Handler>::invoke_handler,
                &op<Handler>::destroy_handler),
        handler_(handler)
    {}

    static bool invoke_handler(op_base *base) {
      return static_cast<op<Handler>*>(base)->handler_();
    }

    static void destroy_handler(op_base *base) {
      delete static_cast<op<Handler>*>(base);
    }

  private:
    Handler handler_;
  };

  typedef std::unordered_map<Descriptor, op_base *> operation_map;
  operation_map operations_;
  op_base *cleanup_operations_;
};

} // namespace core
} // namespace net
} // namespace vjik
#endif
