# GET_MARKS (5)
Retrieves the list of unique marks. While a mark can
be applied to more than one container, this list will
only contain a sincle instance of the mark.

## Payload
Empty

## Reply
The reply is a list of strings:

```json
string[]
```

## Example
```json
[
    "editor",
    "browser"
]
```
