# put-casp
Repozytorium projektu z przedmiotu Programowanie Systemowe i Współbieżne @PUT.

![Application Architecture](./docs/architecture.svg)

### Konwencja typów wiadomości
Z racji, że używamy jedną kolejkę, a wiadomości muszą trafiać do różnych adresatów wprowadzono system typów.
Pole `mtype` struktury wysyłanej przez kolejkę IPC ma 4 najmłodsze bajty równe typowi powiadomienia,
a 2 starsze bajty równe ID adresata.
Na ID adresata przeznaczone są tylko dwa bajty z powodu konieczności możliwości zanegowania całego `mtype`.

#### Struktura wiadomości
```struct message_event
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
#### Sposób przesyłu danych
Wiadomość może zawierać albo tekst, albo liczbę (uint) albo tablicę liczb uint32_t. 
Najczęściej używane:
 - Jeśli zapytanie ma jeden argument (np. LOGIN(ID)), to używamy pola number.
 - Jeśli zapytanie ma kilka argumentów, to podajemy je po kolei do tablicy numbers.
 - Powiadomienia wypełniają pole text.

### Ograniczenia systemowe 

Zalogowanych może być maksymalnie 30 klientów i 30 prodcentów.
**MAX_ID = 30**

Możliwe byłoby UINT16_MAX - 1 (UINT16_MAX jest wykorzystywane jako flaga)

W systemie może być dostępnych maskymalnie 30 typów powiadomień.
**MAX_NOTIFICATION = 30**

Możliwe byłoby UINT32_MAX


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

LOGOUT(ID) - wylogowuje klienta całkowice (można zalogować nowego klienta na to samo ID) -- dodatkowe

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

### Scenariusz komunikacji między klientem a dyspozytorem

#### Logowanie
Klient
```
Wysyła prośbę o logowanie:
mtype - CLI2DISP_LOGIN 
payload.number - identyfikator klienta
```
Dyspozytor
```
Odbiera identyfikator klienta i wysyła odpowiedź:
- Jeśli identyfikator już istnieje:
    mtype - DISP2CLI_LOGIN_FAILED  
    payload - puste

- Jeśli identyfikator jest nowy:  
    mtype - DISP2CLI_LOGIN_OK  
    payload - puste
    Ustawia identyfikator klienta jako zajęty.  
```

#### Pobieranie dostępnych powiadomień
Klient
```
Wysyła żądanie o otrzymanie listy dostępnych powiadomień:
mtype - CLI2DISP_FETCH 
payload.number - identyfikator klienta
```
Dyspozytor
```
Odbiera identyfikator klienta i wysyła odpowiedź:
mtype - DISP2CLI_AVAILABLE_TYPES
payload.numbers[i] = 1 jeśli powiadomienie typu i istnieje, w przeciwnym wypadku 0
```

#### Nowy typ zarejestrowany przez producenta
Klient
```
Klient po otrzymaniu wiadomości o nowym typie wyświetla wiadomość o jego istnieniu.
```
Dyspozytor
```
Rozsyła do wszystkich klientów wiadomość systemową informującą o nowym typie.
mtype - DISP2CLI_NEW_TYPE
payload.number = ID nowego typu
```

#### Subskrypcja powiadomień
Klient
```
Wysyła informacje o subskrypcji typu powiadomienia:
mtype - CLI2DISP_SUBSCRIBE 
payload.numbers[0] - identyfikator klienta
payload.numbers[1] - typ zasubskrybowane powiadomienia
```

#### Rezygnacja z subskrypcji
Klient
```
Wysyła:
mtype - CLI2DISP_UNSUBSCRIBE 
payload.numbers[0] - identyfikator klienta
payload.numbers[1] - typ powiadomienia, z którego chce zrezygnować
```
Dyspozytor
```
Odbiera identyfikator klienta.
Usuwa sybskrypcje klienta dla wskazanego typu powiadomienia,
ale dalej trzyma klienta jako zalogowanego.
```

#### Wylogowanie
Klient
```
Wysyła:
mtype - CLI2DISP_LOGOUT 
payload.number - identyfikator klienta
```
Dyspozytor
```
Odbiera identyfikator klienta:
Ustawia identyfikator klienta jako wolny.
```

#### Odbiór powiadomienia (KOLEJKA POWIADOMIEŃ KLIENTA)
Klient
```
Nasłuchuje wiadomości o mtype z jego ID jako adresatem i typem powiadomienia który subskrybuje.
```
Dyspozytor
```
Po przyjęciu powiadomienia od producenta, sprawdza którzy klienci subskrybują ten typ powiadomień
i rozsyła je do każdego klienta jako oddzielne wiadomości z różnymi adresatami.
```

### Scenariusz komunikacji między producentem a dyspozytorem

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

#### Wysłanie powiadomienia (KOLEJKA POWIADOMIEŃ PRODUCENTA)
Producent
```
Wysyła powiadomienie:
mtype - ID powiadomienia (część adresata jest w tym przypadku ignorowana)
payload.text - treść powiadomienia pobrana przy użyciu scanf()
```
Dyspozytor
```
Odbiera powiadomienie i przekierowuje je do klienta (opisane w sekcji "Scenariusz komunikacji między klientem a dyspozytorem")
```

### Kompilacja kodu
```
make // kompiluje wszystko
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