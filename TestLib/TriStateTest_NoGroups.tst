version_0
Test: "Tri State Buffer Test"
Ports: "Input" "Enable" "Output"

>Set:
    "Enable":0
    "Input":0
>Step 10
>Check:
    "Output":2

>Set:
    "Enable":1
    "Input":0
>Step 10
>Check:
    "Output":0

>Set:
    "Enable":2
    "Input":0
>Step 10
>Check:
    "Output":3

>Set:
    "Enable":3
    "Input":0
>Step 10
>Check:
    "Output":3

>Set:
    "Enable":0
    "Input":1
>Step 10
>Check:
    "Output":2

>Set:
    "Enable":1
    "Input":1
>Step 10
>Check:
    "Output":1

>Set:
    "Enable":2
    "Input":1
>Step 10
>Check:
    "Output":3

>Set:
    "Enable":3
    "Input":1
>Step 10
>Check:
    "Output":3

>Set:
    "Enable":0
    "Input":2
>Step 10
>Check:
    "Output":2

>Set:
    "Enable":1
    "Input":2
>Step 10
>Check:
    "Output":3

>Set:
    "Enable":2
    "Input":2
>Step 10
>Check:
    "Output":3

>Set:
    "Enable":3
    "Input":2
>Step 10
>Check:
    "Output":3

>Set:
    "Enable":0
    "Input":3
>Step 10
>Check:
    "Output":2

>Set:
    "Enable":1
    "Input":3
>Step 10
>Check:
    "Output":3

>Set:
    "Enable":2
    "Input":3
>Step 10
>Check:
    "Output":3

>Set:
    "Enable":3
    "Input":3
>Step 10
>Check:
    "Output":3