# put-casp
Repozytorium projektu z przedmiotu Programowanie Systemowe i Współbieżne @PUT.

![Application Architecture](./docs/architecture.svg)

### Konwencja typów wiadomości
Z racji, że używamy jedną kolejkę, a wiadomości muszą trafiać do różnych adresatów wprowadzono system typów.
Pole `mtype` struktury wysyłanej przez kolejkę IPC ma 4 najmłodsze bajty równe typowi powiadomienia,
a 2 starsze bajty równe ID adresata.
Na ID adresata przeznaczone są tylko dwa bajty z powodu konieczności możliwości zanegowania całego `mtype`.

## Struktura wiadomości systemowej
```struct system_message
{
    long mtype;
    union
    {
        char text[MAX_MESSAGE_SIZE];
        unsigned int number;
        uint32_t numbers[MAX_MESSAGE_SIZE / sizeof(uint32_t)];
    } payload;
};```

### Wiadomości systemowe producenta 
```
LOGIN(ID, TYPE) - loguje się do dyspozytora podając swoje ID i typ powiadomień które będzie wysyłał.
```

### Wiadomości systemowe dyspozytora do producenta
```
LOGIN_OK() - zalogowano pomyślnie

LOGIN_FAILED() - błąd logowania, np. osiągnieto już maksymalną ilość producentów
```

### Wiadomości systemowe klienta
```
LOGIN(ID) - loguje się do dyspozytora podając swoje ID

FETCH() - prośba do dyspozytora o zwrócenie aktualnie zarejestrowanych typów wiadomości

SUBSCRIBE(ID, TYPE) - klient podaje swoje ID i typ powiadomienia który chciałby dostawać w przyszłości

UNSUBSCRIBE(ID, TYPE) - klient podaje swoje ID i typ powiadomienia, którego nie chce dłużej subskrybować
```

### Wiadomości systemowe dyspozytora do klienta
```
LOGIN_OK() - zalogowano pomyślnie

LOGIN_FAILED() - błąd logowania, np. osiągnięto już maksymalną ilość klientw

AVAILABLE_TYPES(TYPE[]) - aktualnie zarejestrowane typy powiadomień 

NEW_TYPE(TYPE) - nowy typ zarejestrowany przez producentów
```

### Kompilacja kodu
```
make
make client
make producer
make dispatcher
```

### Kompilacja kodu z flagą DEBUG
To samo co normalnie, ale z dodanym `DEBUG=1`
```
make client DEBUG=1
```

### Wyczyszczenie kolejek
Polecenie `make rm_ipcs` usuwa wszystkie kolejki stworzone przez cały system.