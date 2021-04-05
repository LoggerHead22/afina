#include <afina/concurrency/Executor.h>

namespace Afina {
namespace Concurrency {

void perform(Executor *executor){

    while(true){
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(executor->mutex);
            executor->free_workers++;
            while(executor->tasks.empty() && executor->state==Executor::State::kRun){ //пока очередь пуста и пул работает

                auto status = executor->empty_condition.wait_for(lock, std::chrono::milliseconds(executor->_idle_time));

                if(status == std::cv_status::timeout && executor->threads.size() > executor->_low_watermark){ //Если разбудил таймаут и потоков больше чем нужно
                    DeleteThread(executor);
                    executor->free_workers--;
                    return;
                }
            }

            if(executor->state != Executor::State::kRun && executor->tasks.empty()){ //Если пул останавливается и задач больше нет
                DeleteThread(executor);
                if(executor->threads.empty()){
                    executor->stop_condition.notify_one();
                }
                return;
            }

            task = std::move(executor->tasks.front());
            executor->tasks.pop_front();
            executor->free_workers--;
        }
        task();
    }
}

void DeleteThread(Executor *executor){
    auto id = std::this_thread::get_id();
    auto iter = std::find_if(executor->threads.begin(), executor->threads.end(), [=](std::thread &t) { return (t.get_id() == id); });

    if(iter != executor->threads.end()){
        iter->detach(); //открепляем поток
        executor->threads.erase(iter); // удаляем из пула
    }
}

void Executor::AddNewThread(){
    threads.push_back(std::thread(perform, this));
}


Executor::Executor(std::string name, size_t low_watermark=2, size_t hight_watermark=8,
         size_t max_queue_size=128, size_t idle_time=2000)
    : _low_watermark(low_watermark),
      _hight_watermark(hight_watermark),
     _max_queue_size(max_queue_size),
     _idle_time(idle_time)
    {

    if(_low_watermark < 1){
        _low_watermark = 1;
    }

    if(_hight_watermark < _low_watermark){
        _hight_watermark = _low_watermark;
    }
}

void Executor::Start(){
    std::unique_lock<std::mutex> lock(mutex);

    for(size_t i=0; i < _low_watermark; i++){
        AddNewThread();
    }
    state=State::kRun;
}

void Executor::Stop(bool await){
    {
        std::unique_lock<std::mutex> lock(mutex);
        state = State::kStopping;
    }
    empty_condition.notify_all();

    {
        std::unique_lock<std::mutex> lock(mutex);
        if(await){
            while(!threads.empty()){
                stop_condition.wait(lock);
            }
        }
        state = State::kStopped;
    }
}

Executor::~Executor(){
    std::unique_lock<std::mutex> lock(mutex);
    if(state != State::kStopped){
        lock.unlock();
        Stop(true);
    }
}

}
} // namespace Afina
