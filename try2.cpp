#include <iostream>
#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <queue>
#include <random>

HANDLE hEvent;
CRITICAL_SECTION readerCS;
CRITICAL_SECTION writerCS;
std::random_device rd;  // Источник случайности
std::mt19937 gen(rd());  // Генератор случайных чисел (Мерсеннский твистер)
std::uniform_int_distribution<> dis(0, 99);  // Диапазон от 0 до 99

class Buffer
{
protected: 
    std::deque<int> q;
    size_t size_;
public: 
    Buffer(size_t size) { size_ = size; }
    void push(int value) {
        if (q.size() >= size_) {
        //    printf("bolshe 10 i pishet\n");//чисто для откладки
            return;
        }
        q.push_back(value);
        //printf("dobavilo %d\n", value);
    }
    void pop() {
        if (!q.empty()) 
        {
            printf("%d\n", q.front());
            q.pop_front();
            return;
        }
        //else printf("pust u pitayetsya vivesti\n");//чисто для откладки
    }
    int front() {
        return q.front();
    }
    bool empty() {
        return q.empty();
    }
    size_t size() {
        return q.size();
    }
};

Buffer bufer(10);

DWORD WINAPI Print(LPVOID lpParam)
{
    int time = *reinterpret_cast<int*>(lpParam);
    WaitForSingleObject(hEvent, INFINITE);

    for (;;)
    {
        EnterCriticalSection(&readerCS); // обеспечивает не более одного читателя в данный момент
            //читаем спокойно, так как в добавляет в конец, а читаем с начала
                            //                            
                            //
            bufer.pop();    //чтение
                            //
                            //
         LeaveCriticalSection(&readerCS);
         std::this_thread::sleep_for(std::chrono::milliseconds(time));
    }
    return 0;
}
DWORD WINAPI GenerateNums(LPVOID lpParam) 
{
    int time = *reinterpret_cast<int*>(lpParam); 
    WaitForSingleObject(hEvent, INFINITE);

    for (;;)
    {
        int num = dis(gen);
        EnterCriticalSection(&writerCS); // обеспечивает не более одного писателя в данный момент
                            //                            
                            //
            bufer.push(num);//запись
                            //
                            //
        LeaveCriticalSection(&writerCS);
        std::this_thread::sleep_for(std::chrono::milliseconds(time));
    }
    return 0;
}

// Записываем пока буфер не полный && читаем пока не пустой && можем читать пока пишет && не можем (читать/читать || писать/писать)

int main() 
{
    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent == NULL) {
        std::cerr << "Failed to create event." << std::endl;
        return 1;
    }

    InitializeCriticalSection(&readerCS);
    InitializeCriticalSection(&writerCS);

    int time1 = 300;//читатель
    int time2 = 500;//писатель1
    int time3 = 500;//писатель2

    HANDLE reader = CreateThread(NULL, 0, Print, &time1, CREATE_SUSPENDED, NULL);
    HANDLE writer1 = CreateThread(NULL, 0, GenerateNums, &time2, CREATE_SUSPENDED, NULL);
    HANDLE writer2 = CreateThread(NULL, 0, GenerateNums, &time3, CREATE_SUSPENDED, NULL);

    ResumeThread(writer1);
    ResumeThread(writer2);
    ResumeThread(reader);

    SetEvent(hEvent);

    WaitForSingleObject(writer1, INFINITE);
    WaitForSingleObject(writer2, INFINITE);
    WaitForSingleObject(reader, INFINITE);
    
    CloseHandle(writer1);
    CloseHandle(writer2);
    CloseHandle(reader);

    CloseHandle(hEvent);


    DeleteCriticalSection(&readerCS);
    DeleteCriticalSection(&writerCS);

    return 0;
}

  //Ответы на вопросы:
  //1 : Поток - объект операционной системы, наименьшая единица выполнения внутри процесса, имеющая собственное состояние, регистры, стек и программный счетчик; 
  //внутри процесса потоки разделяют его ресурсы и память, выполнятся раздельно друг от друга; позволяет эффективно пользоваться многозадачностью в многопроцессорных
  //системах
  //Процесс - объект операционной системы, являющийся совокупностью взаимосвязанных системных ресурсов на основе отдельного и независимого виртуального пространства
  //в контексте которой происходит выполнение потоков; элементами являются : адресное пространство, глобальные переменные, регистры, стек, открытые файлы...
  //Владеет следующими ресурсами : образ машинного кода, ассоциированного с программой; памятью(исполняемый код, входные и выходные данные, стек вызовов, кучу); 
  //дескрипторами ресурсов; файловыми дескрипторами; атрибутами безопасности; состоянием процессора.
  //2 : у меня create_suspended и resume, поэтому все вместе
  //3 : дисциплина - планировщик; у меня по принципу fifo(в том числе и вход в критическую секцию)
  //4: Записываем пока буфер не полный && читаем пока не пустой && можем читать пока пишет && не можем(читать / читать || писать / писать)
  //5 : Выбран двунаправленная очередь : удобнее обычной.Стек и массив труднее для реализации
  //6 : Для очереди нужно синхронизировать повторные запросы на чтение или запись(читать / читать || писать / писать), так как читаем с начала и записываем в конец,
  //поэтому читатель избегает доступ к области памяти пишущего писателя, писатель же избегает запись в область памяти читающего читателя
  //7 : Мьютекс отличается от бинарного семафора тем, что в мьютексе критическую секцию открывает и закрывает один поток, в БС могут другие
  //БС от ивентов : БС подразумевает, что какой - то поток входит в критическую секцию и он же или другой поток не вызовет release(), setevent же можно вызвать 
  //и без этого