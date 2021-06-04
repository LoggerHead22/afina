#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

void Engine::Store(context &ctx) {
    char c_s;
    ctx.Hight = &c_s;
    std::size_t stack_size;

    if(ctx.Low > ctx.Hight){
        std::swap(ctx.Low, ctx.Hight);
    }
    stack_size = ctx.Hight - ctx.Low;

    //std::cout<<"Count of bytes: "<<stack_size<<&c_s<<" "<<ctx.Low<<std::endl;
    if(std::get<1>(ctx.Stack) < stack_size || std::get<1>(ctx.Stack) >= 2 * stack_size){
        delete[] std::get<0>(ctx.Stack);
        std::get<1>(ctx.Stack) = stack_size;
        std::get<0>(ctx.Stack) = new char[stack_size];
    }
    std::memcpy(std::get<0>(ctx.Stack), ctx.Low, stack_size);
}

void Engine::Restore(context &ctx) {
    char c;
    if(ctx.Low > ctx.Hight){
        std::swap(ctx.Low, ctx.Hight);
    }

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
    //std::cout<<"Sched: "<<routine_<<" "<<cur_routine<<std::endl;
    if(routine_ == nullptr){
        yield();
    }else{
        context *coroutine = static_cast<context*>(routine_);

        if(coroutine == cur_routine){
            return;
        }

        if(setjmp(cur_routine->Environment) <= 0){
            //std::cout<<cur_routine<<" wake up "<<routine_<<std::endl;
            Store(*cur_routine);
            Restore(*coroutine);
        }
        //std::cout<<cur_routine<<" is waking up "<<std::endl;
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
