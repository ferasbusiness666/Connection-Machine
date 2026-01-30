version_0
Test: "Tri State Buffer Test"
Ports: "Input", "Enable", "Output"
Group "0 0 Z":
    >Set:
        "Enable":0
        "Input":0
    >Step 10
    >Check:
        "Output":2
Group "0 1 0":
    >Set:
        "Enable":1
        "Input":0
    >Step 10
    >Check:
        "Output":0
Group "0 Z X":
    >Set:
        "Enable":2
        "Input":0
    >Step 10
    >Check:
        "Output":3
Group "0 X X":
    >Set:
        "Enable":3
        "Input":0
    >Step 10
    >Check:
        "Output":3
Group "1 0 Z":
    >Set:
        "Enable":0
        "Input":1
    >Step 10
    >Check:
        "Output":2
Group "1 1 1":
    >Set:
        "Enable":1
        "Input":1
    >Step 10
    >Check:
        "Output":1
Group "1 Z X":
    >Set:
        "Enable":2
        "Input":1
    >Step 10
    >Check:
        "Output":3
Group "1 X X":
    >Set:
        "Enable":3
        "Input":1
    >Step 10
    >Check:
        "Output":3
Group "Z 0 Z":
    >Set:
        "Enable":0
        "Input":2
    >Step 10
    >Check:
        "Output":2
Group "Z 1 X":
    >Set:
        "Enable":1
        "Input":2
    >Step 10
    >Check:
        "Output":3
Group "Z Z X":
    >Set:
        "Enable":2
        "Input":2
    >Step 10
    >Check:
        "Output":3
Group "Z X X":
    >Set:
        "Enable":3
        "Input":2
    >Step 10
    >Check:
        "Output":3
Group "X 0 Z":
    >Set:
        "Enable":0
        "Input":3
    >Step 10
    >Check:
        "Output":2
Group "X 1 X":
    >Set:
        "Enable":1
        "Input":3
    >Step 10
    >Check:
        "Output":3
Group "X Z X":
    >Set:
        "Enable":2
        "Input":3
    >Step 10
    >Check:
        "Output":3
Group "X X X":
    >Set:
        "Enable":3
        "Input":3
    >Step 10
    >Check:
        "Output":3