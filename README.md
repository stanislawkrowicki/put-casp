# put-casp
Repozytorium projektu z przedmiotu Programowanie Systemowe i Współbieżne @PUT.

![Application Architecture](./docs/architecture.svg)

### Konwencja wiadomości systemowych
Wiadomości systemowe i zwykłe są wysyłane w odmienny sposób.
Z racji, że używamy jedną kolejkę, a potrzebujemy mieć możliwość przekazania danych z dyspozytora tylko do wybranego klienta/producenta, to `TYPE` wiadomości scala dwie wartości - ID odbiorcy (2 bajty) do którego ma dotrzeć wiadomość i typ komunikatu (4 bajty).
Także, gdy `TYPE = 0xAAAA0000FFFF`, to `0xAAAA` jest ID odbiorcy, a `0x0000FFFF` to typ komunikatu.

### Wiadomości systemowe producenta 
```
LOGIN(ID, TYPE) - loguje się do dyspozytora podając swoje ID i typ powiadomień które będzie wysyłał.

REGISTER_MESSAGE(TYPE) - oznajmia dyspozytorowi, że od teraz będzie wysyłał dodatkowo również eventy TYPE.
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

