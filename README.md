# put-casp
Repozytorium projektu z przedmiotu Programowanie Systemowe i Współbieżne @PUT.

![Application Architecture](./docs/architecture.svg)

### Konwencja typów wiadomości
Z racji, że używamy jedną kolejkę, a wiadomości muszą trafiać do różnych adresatów wprowadzono system typów.
Pole `mtype` struktury wysyłanej przez kolejkę IPC ma 4 najmłodsze bajty równe typowi powiadomienia,
a 2 starsze bajty równe ID adresata.
Na ID adresata przeznaczone są tylko dwa bajty z powodu konieczności możliwości zanegowania całego `mtype`.

#### Struktura wiadomości systemowej
```struct system_message
{
    long mtype;
    union
    {
        char text[MAX_MESSAGE_SIZE];
        unsigned int number;
        uint32_t numbers[MAX_MESSAGE_SIZE / sizeof(uint32_t)];
    } payload;
};
```

### Ograniczenia systemowe
```
Zalogowanych może być maksymalnie 30 klientów i 30 prodcentów.
MAX_ID = 30

W systemie może być dostępnych maskymalnie 30 typów powiadomień.
MAX_NOTIFICATION = 30

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

### Scenariusza komunikacji między klientem a dyspozytorem

#### Logowanie
Klient
```
Wysyła prośbę o logowanie:
mtype - CLI2DISP_LOGIN 
payload - identyfikator klienta
```
Dyspozytor
```
Odbiera identyfikator klienta i wysyła odpowiedź:
- Jeśli identyfikator już istnieje:
    mtype - DISP2CLI_LOGIN_FAILED  
    payload - komunikat o błędzie (np. "ID zajęte")  

- Jeśli identyfikator jest nowy:    
    mtype - DISP2CLI_LOGIN_OK  
    payload - potwierdzenie logowania
    Ustawia identyfikator klienta jako zajęty.  
```

#### Pobieranie dostępnych powiadomień
Klient
```
Wysyła żądanie o otrzymanie listy dostępnych powiadomień:
mtype - CLI2DISP_FETCH 
payload - identyfikator klienta
```
Dyspozytor
```
Odbiera identyfikator klienta i wysyła odpowiedź:
mtype - DISP2CLI_AVAILABLE_TYPES
payload[0] - identyfikator klienta
payload[1] - lista typów powiadomień (wartośc 1 informuje o dostępności danego typu)
```

#### Przekazywanie powiadomień
Klient
```
Wysyła informacje o subskrypcji typu powiadomienia:
mtype - CLI2DISP_SUBSCRIBE 
payload[0] - identyfikator klienta
payload[1] - typ zasubskrybowane powiadomienia
```
Dyspozytor
```
Odbiera identyfikator klienta i jaki typ chce otrzymywać i wysyła odpowiedź:
mtype - DISP2CLI_
payload - ?
```
#### Wylogowanie
Klient
```
Wysyła :
mtype - CLI2DISP_LOGOUT 
payload - identyfikator klienta
```
Dyspozytor
```
Odbiera identyfikator klienta:
Ustawia idnetyfikator klienta jako wolny.
```
#### Rezygnacja z subskrypcji
Klient
```
Wysyła :
mtype - CLI2DISP_UNSUBSCRIBE 
payload[0] - identyfikator klienta
payload[1] - typ powiadomienia, z którego chce zrezygnować
```
Dyspozytor
```
Odbiera identyfikator klienta:
Usuwa sybskrypcje klienta dla wskazanego typu powiadomienia.
```
### Scenariusza komunikacji między producentem a dyspozytorem

#### Logowanie
Producent
```
Wysyła prośbę o logowanie:
mtype - PROD2DISP_LOGIN 
payload[0] - identyfikator producenta
payload[1] - typ produkowane przez producenta powiadomienia
```
Dyspozytor
```
Odbiera identyfikator klienta i wysyła odpowiedź:
- Jeśli identyfikator już istnieje:
    mtype - DISP2PROD_LOGIN_FAILED  
    payload - komunikat o błędzie (np. "ID zajęte")  

- Jeśli identyfikator jest nowy:    
    mtype - DISP2PROD_LOGIN_OK  
    payload - potwierdzenie logowania
    Ustawia identyfikator klienta jako zajęty.  
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