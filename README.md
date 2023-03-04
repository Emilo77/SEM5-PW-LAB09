# Laboratorium - biblioteka pthreads

### Dołączone pliki

Do laboratorium zostały dołączone pliki:
- [.clang-format](.clang-format)
- [.clang-tidy](.clang-tidy)
- [CMakeLists.txt](CMakeLists.txt)
- [err.c](err.c)
- [err.h](err.h)
- [primes.c](primes.c)
- [producer-consumer.c](producer-consumer.c)
- [readers-writers-template.c](readers-writers-template.c)

## Implementacje wątków

Omówimy wątki zgodne ze standardem POSIX, czyli POSIX Threads (lub **pthreads**).

Aby skompilować program używający biblioteki **pthreads**, należy upewnić się, że jest linkowana (w przykładowym [CMakeLists.txt](CMakeLists.txt) robi to `target_link_libraries`; przy ręcznej kompilacji robi to parametr `-lpthread`).

## Tworzenie wątku

Do tworzenia wątków służy funkcja

```c
int pthread_create(pthread_t* thread, pthread_attr_t* attr, void* (*func)(void*), void* arg)
```

Funkcja ta tworzy wątek, którego kod wykonywalny znajduje się w funkcji podanej jako argument `func`. Funkcja `func` powinna przyjmować jeden argument typu `void*` (czyli dowolny wskaźnik) i zwracać też wartość typu `void*`. Wątek jest uruchamiany z argumentem `arg`, a informacja o wątku (deskryptor wątku) jest umieszczana w strukturze `thread`.

Stworzony przez funkcję `pthread_create()` wątek działa do chwili, gdy funkcja będąca treścią wątku zostanie zakończona przez `return`, wywołanie funkcji `pthread_exit()` lub po prostu zostanie wykonana ostatnia instrukcja funkcji.

Argument `attr` zawiera atrybuty z jakimi chcemy stworzyć wątek. Do obsługi atrybutów wątku służą funkcje `pthread_attr_*`. Są to między innymi:

```c
int pthread_attr_init(pthread_attr_t* attr); 
/* inicjowanie obiektu attr domyślnymi wartościami */

int pthread_attr_destroy(pthread_attr_t* attr);
/* niszczenie obiektu attr */

int pthread_attr_setdetachstate(pthread_attr_t* attr, int state); 
/* ustawianie atrybutu detachstate, 
który określa, czy można czekać na zakończenie wątku; 
możliwe wartości state to PTHREAD_CREATE_JOINABLE (domyślna)
i PTHREAD_CREATE_DETACHED */
```

## Czekanie na zakończenie wątku

Podobnie jak w przypadku procesów program główny może oczekiwać na zakończenie się wątków, które stworzył. Służy do tego funkcja

```c
int pthread_join(pthread_t thread, void** status)
```

Co najwyżej jeden wątek może czekać na zakończenie danego wątku. Wywołanie `pthread_join()` dla wątku, na który czeka już inny wątek, zakończy się błędem.

Można czekać tylko na wątki, które zostały stworzone z atrybutem `detachstate` ustawionym na `PTHREAD_CREATE_JOINABLE`. Po uruchomieniu wątku można jeszcze zmienić wartość tego atrybutu poprzez wywołanie funkcji `pthread_detach()`, która powoduje, że na wątek nie można będzie oczekiwać. Taki wątek będziemy nazywać odłączonym.

Jeśli wątek przejdzie w stan odłączony to zaraz po zakończeniu tego wątku czyszczone są wszystkie związane z nim struktury w pamięci. Jeśli jednak wątek nie przejdzie do stanu odłączony to informacja o jego zakończeniu pozostanie w pamięci do czasu, aż inny wątek wykona funkcję `pthread_join()` dla zakończonego wątku.

Przeczytaj przykład [primes.c](primes.c) ilustrujący użycie podstawowych funkcji.

Wywołaj `primes` z parametrem i bez parametru. Co stanie się jeśli wątki zostaną utworzone jako odłączone?

Zwróć uwagę na inną niż dotychczas poznana obsługę błędów. Konwencja obsługi błędów w bibliotece **pthreads** jest taka, że funkcje zwracają zero w przypadku powodzenia, a w przypadku błędu zwracają kod błędu (kody są takie jak w przypadku `errno`). W przykładach obsługujemy je makrem `ASSERT_ZERO` zdefiniowanym w [err.h](err.h). Różnica wynika stąd, że we wcześniejszcych wersjach systemu Linux zmienna zmienna `errno` była wspólna dla wszystkich watków — jedna na proces. Nie dawała wiarygodnej informacji co do przyczyny błędu, ponieważ kod błędu mógł być nadpisany przez inny wątek. Aktualnie w Linuksie `errno` może być zdefiniowane jako makro i można go bezpiecznie używać w programach wielowątkowych, więc tradycyjna obsługa błędów byłaby również poprawna. Więcej na ten temat: [https://linux.die.net/man/3/errno](https://linux.die.net/man/3/errno)

## Synchronizacja wątków
### Muteksy

Podstawowe funkcje operujące na muteksach to:
```c
int pthread_mutex_init(pthread_mutex_t* mutex, pthread_mutexattr_t* attr);
/* inicjowanie muteksa */

int pthread_mutex_destroy(pthread_mutex_t* mutex);
/* niszczenie muteksa */

int pthread_mutex_lock(pthread_mutex_t* mutex);
/* zdobycie muteksa */

int pthread_mutex_trylock(pthread_mutex_t* mutex);
/* próba zdobycia muteksa */

int pthread_mutex_unlock(pthread_mutex_t* mutex);
/* oddanie muteksa */
```

Mutex może być w dwóch stanach: może być wolny albo może być w posiadaniu pewnego wątku. Wątek, który chce zdobyć muteks wykonuje operację `pthread_mutex_lock`. Jeśli mutex jest wolny, to operacja kończy się powodzeniem i wątek staje się właścicielem muteksa. Jeśli jednak mutex jest w posiadaniu innego wątku, to operacja powoduje wstrzymanie wątku aż do momentu kiedy muteks będzie wolny.

Operacja `pthread_mutex_trylock` działa podobnie, z wyjątkiem tego, że zwraca błąd w sytuacji, gdy wątek miałby być wstrzymany.

Wątek, który jest w posiadaniu muteksa może go oddać wykonując operację `pthread_mutex_unlock`. Powoduje to obudzenie pewnego wątku oczekującego na operacji lock, jeśli taki jest i przyznanie mu muteksa. Jeśli nikt nie czeka, to muteks staje się wolny.

Zwróćmy uwagę, że `pthread_mutex_unlock` może wykonać tylko ten wątek, który posiada muteks. Implementacja w Linuksie nie narzuca powyższego wymogu, ale używanie operacji `pthread_mutex_lock` i `pthread_mutex_unlock` w inny sposób powoduje nieprzenośność programu.

Biblioteka **pthreads** udostępnia trzy rodzaje muteksów (rodzaj wybieramy ustawiając odpowiedni atrybut podczas tworzenia wątku), które różnie reagują w sytuacji, gdy wątek wykonuje lock na muteksie, który już posiada. Po szczegóły odsyłamy do `man pthread_mutexattr_settype`.

### Zmienne warunkowe

Zmienne warunkowe z biblioteki odpowiadają zmiennym warunkowym monitora. Operacje `wait()` i `signal()` to odpowiednio `pthread_cond_wait()` i `pthread_cond_signal()`.

Aby zasymulować działanie monitora należy użyć muteksa zapewniającgo wzajemne wykluczanie przy wykonywaniu funkcji monitora. Każda taka funkcja powinna zaczynać się od operacji `pthread_mutex_lock()` i kończyć wykonaniem `pthread_mutex_unlock()`.

```c
int pthread_cond_init(pthread_cond_t* cond, pthread_condattr_t* cond_attr);
/* inicjowanie zmiennej warunkowej (argument cond_attr jest zawsze NULL) */

int pthread_cond_destroy(pthread_cond_t* cond);
/* niszczenie zmiennej warunkowej */

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
/* funkcja atomowo zwalnia mutex, który musiał być wcześniejw posiadaniu wątku
i zawiesza wątek na zmiennej warunkowej cond (chwilowe wyjście z monitora);
po obudzeniu wątek ponownie walczy o zdobycie mutex i zdobywa go
zanim zwróci z pthread_cond_wait() */

int pthread_cond_signal(pthread_cond_t* cond);
/* jeśli żaden wątek nie czekał na kolejce cond, to nic się nie dzieje;
w przeciwnym przypadku jeden z czekających wątków jest budzony. */
```

Standardowy scenariusz użycia zmiennej warunkowej wygląda następująco:

```c
pthread_mutex_lock(&mut);
...
while (warunek) 
    pthread_cond_wait(&cond, &mut);
...
pthread_mutex_unlock(&mut);
```

Zwróćmy uwagę na użycie pętli zamiast warunku. Są ku temu dwa powody. Monitory poznane na ćwiczeniach gwarantują zwalnianie procesów oczekujących na zmiennych warunkowych przed wpuszczeniem nowych procesów do monitora — dlatego instrukcja `if` jest wystarczająca. W przypadku biblioteki **pthreads** takiej gwarancji nie ma, więc po zwolnieniu muteksa do monitora może wejść inny wątek i ,,zepsuć'' warunek, na który oczekiwał budzony wątek. Obudzony wątek musi zatem ponownie sprawdzić, czy warunek jest spełniony. Drugim powodem jest spontaniczne, nieuzasadnione budzenie wątków czekających na zmiennej warunkowej 
([patrz: https://linux.die.net/man/3/pthread_cond_wait](https://linux.die.net/man/3/pthread_cond_wait)).

Przeanalizuj przykład [producer-consumer.c](producer-consumer.c)

Oprócz powyższych operacji na zmiennych warunkowych dostępne są również dwie dodatkowe:
```c
pthread_cond_timedwait();
/* czekanie tylko przez określony czas */
pthread_cond_broadcast();
/* budzenie wszystkie wątków czekających w kolejce */
```


## Ćwiczenie punktowane

W pliku [readers-writers-template.c](readers-writers-template.c) znajdziesz schemat rozwiązania problemu czytelników i pisarzy. Uzupełnij go do poprawnego rozwiązania.



