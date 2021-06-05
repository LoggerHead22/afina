#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char c_s;
    //ctx.Low = &c_s;
    std::size_t stack_size;

    if(ctx.Low == ctx.Hight){ //только для первого входа корутины
        if(&c_s <= ctx.Low){
            stack_grow_down = false;
        }else if(&c_s >= ctx.Hight){
            stack_grow_down = true;
        }
    }

    if(!stack_grow_down){
        ctx.Low = &c_s; //grow up -> update low
    }else{
        ctx.Hight = &c_s; //grow down -> update hight
    }

    stack_size = ctx.Hight - ctx.Low;

    if(std::get<1>(ctx.Stack) < stack_size || std::get<1>(ctx.Stack) >= 2 * stack_size){
        delete[] std::get<0>(ctx.Stack);
        std::get<1>(ctx.Stack) = stack_size;
        std::get<0>(ctx.Stack) = new char[stack_size];
    }
    std::memcpy(std::get<0>(ctx.Stack), ctx.Low, stack_size);
}

void Engine::Restore(context &ctx) {
    char c;

    if(&c >= ctx.Low && &c <= ctx.Hight){
        Restore(ctx);
    }
    cur_routine = &ctx;
    std::memcpy(ctx.Low, std::get<0>(ctx.Stack), ctx.Hight - ctx.Low);
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    //there is no more active coroutine other than the current one
    //or there is no active coroutine
    if((alive == cur_routine && cur_routine->next == nullptr) || alive ==nullptr){
        return;
    }

    context* new_coro = alive;
    if(cur_routine == alive){
        new_coro = alive->next;
    }
    if(cur_routine != idle_ctx){
        if(setjmp(cur_routine->Environment) > 0){
            return;
        }
        Store(*cur_routine);
    }
    Restore(*new_coro);
}

void Engine::sched(void  *routine_) {

    if(routine_ == nullptr){
        yield();
    }else{
        context *coroutine = static_cast<context*>(routine_);

        if(coroutine == cur_routine){
            return;
        }

        if(cur_routine != idle_ctx){
            if(setjmp(cur_routine->Environment) > 0){
                return;
            }
            Store(*cur_routine);
        }
        Restore(*coroutine);
    }
}

void Engine::block(void *coro){
    context* coroutine;
    if(coro == nullptr){
        coroutine = cur_routine;
    }else{
        coroutine = static_cast<context*>(coro);
    }

    swap_list(coroutine, alive, blocked);
    if(coroutine == cur_routine){
        yield();
    }
}

void Engine::unblock(void *coro){
    context* coroutine = static_cast<context*>(coro);
    swap_list(coroutine, blocked, alive);
}

void Engine::swap_list(context* coroutine, context* src, context* dst){
    //src list
    if(coroutine->prev != nullptr){
        coroutine->prev->next = coroutine->next;
    }else{
        src = coroutine->next;
        if(coroutine->next){
            coroutine->next->prev = nullptr;
        }
    }

    if(coroutine->next != nullptr){
        coroutine->next->prev = coroutine->prev;
    }else{
        if(coroutine->prev){
            coroutine->prev->next = nullptr;
        }
    }

    //dst list
    coroutine->next = dst;
    coroutine->prev = nullptr;
    dst = coroutine;
    if (coroutine->next != nullptr) {
        coroutine->next->prev = coroutine;
    }
}


} // namespace Coroutine
} // namespace Afina
