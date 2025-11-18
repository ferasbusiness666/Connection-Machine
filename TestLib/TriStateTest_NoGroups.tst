version_0
Test: "Tri State Buffer Test"
Ports: "Input" "Enable" "Output"

>Set:
    "Input":0
    "Enable":0
>Step 10
>Check:
    "Output":2

>Set:
    "Input":1
    "Enable":0
>Step 10
>Check:
    "Output":0

>Set:
    "Input":2
    "Enable":0
>Step 10
>Check:
    "Output":3

>Set:
    "Input":3
    "Enable":0
>Step 10
>Check:
    "Output":3

>Set:
    "Input":0
    "Enable":1
>Step 10
>Check:
    "Output":2

>Set:
    "Input":1
    "Enable":1
>Step 10
>Check:
    "Output":1

>Set:
    "Input":2
    "Enable":1
>Step 10
>Check:
    "Output":3

>Set:
    "Input":3
    "Enable":1
>Step 10
>Check:
    "Output":3

>Set:
    "Input":0
    "Enable":2
>Step 10
>Check:
    "Output":2

>Set:
    "Input":1
    "Enable":2
>Step 10
>Check:
    "Output":3

>Set:
    "Input":2
    "Enable":2
>Step 10
>Check:
    "Output":3

>Set:
    "Input":3
    "Enable":2
>Step 10
>Check:
    "Output":3

>Set:
    "Input":0
    "Enable":3
>Step 10
>Check:
    "Output":2

>Set:
    "Input":1
    "Enable":3
>Step 10
>Check:
    "Output":3

>Set:
    "Input":2
    "Enable":3
>Step 10
>Check:
    "Output":3

>Set:
    "Input":3
    "Enable":3
>Step 10
>Check:
    "Output":3