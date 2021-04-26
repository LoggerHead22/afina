#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char c;
    ctx.Hight = &c;

    std::size_t stack_size = ctx.Hight - ctx.Low;
    if(std::get<1>(ctx.Stack) < stack_size || 2 * std::get<1>(ctx.Stack) >= stack_size){
        delete[] std::get<0>(ctx.Stack);
        std::get<1>(ctx.Stack) = stack_size;
        std::get<0>(ctx.Stack) = new char[stack_size];
    }

    std::memcpy(ctx.Low, std::get<0>(ctx.Stack), stack_size);
}

void Engine::Restore(context &ctx) {
    char c;
    if(&c >= ctx.Low && &c <= ctx.Hight){
        Restore(ctx);
    }
    std::memcpy(std::get<0>(ctx.Stack), ctx.Low, ctx.Hight - ctx.Low);
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    //there is no more active coroutine other than the current one
    if(!(alive == cur_routine && cur_routine->next == nullptr)){
        return;
    }

    if(setjmp(cur_routine->Environment) <= 0){
        Store(*cur_routine);
        context* new_coro = alive;

        if(cur_routine == alive){
            new_coro = alive->next;
        }
        Restore(*new_coro);
    }
}

void Engine::sched(void  *routine_) {
    if(routine_ == nullptr){
        yield();
    }else{
        context *coroutine = (context*)routine_;
        //context *coroutine = routine_;
        if(coroutine == cur_routine){
            return;
        }

        if(setjmp(cur_routine->Environment) <= 0){
            Store(*cur_routine);
            Restore(*coroutine);
        }
    }
}

void Engine::block(void *coro){
    context* coroutine;
    if(coro == nullptr){
        coroutine = cur_routine;
    }else{
        coroutine = (context*)coro;
    }

    swap_list(coroutine, alive, blocked);
    if(coroutine == cur_routine){
        yield();
    }
}

void Engine::unblock(void *coro){
    context* coroutine = (context*)coro;
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
