version_0
Test: "Tri State Buffer Test"
Ports: "Input", "Enable", "Output"
"0 0 Z" {
    >Set:
        "Enable":0
        "Input":0
    >Step 10
    >Check:
        "Output":2
}
"0 1 0" {
    >Set:
        "Enable":1
        "Input":0
    >Step 10
    >Check:
        "Output":0
}
"0 Z X" {
    >Set:
        "Enable":2
        "Input":0
    >Step 10
    >Check:
        "Output":3
}
"0 X X" {
    >Set:
        "Enable":3
        "Input":0
    >Step 10
    >Check:
        "Output":3
}
"1 0 Z" {
    >Set:
        "Enable":0
        "Input":1
    >Step 10
    >Check:
        "Output":2
}
"1 1 1" {
    >Set:
        "Enable":1
        "Input":1
    >Step 10
    >Check:
        "Output":1
}
"1 Z X" {
    >Set:
        "Enable":2
        "Input":1
    >Step 10
    >Check:
        "Output":3
}
"1 X X" {
    >Set:
        "Enable":3
        "Input":1
    >Step 10
    >Check:
        "Output":3
}
"Z 0 Z" {
    >Set:
        "Enable":0
        "Input":2
    >Step 10
    >Check:
        "Output":2
}
"Z 1 X" {
    >Set:
        "Enable":1
        "Input":2
    >Step 10
    >Check:
        "Output":3
}
"Z Z X" {
    >Set:
        "Enable":2
        "Input":2
    >Step 10
    >Check:
        "Output":3
}
"Z X X" {
    >Set:
        "Enable":3
        "Input":2
    >Step 10
    >Check:
        "Output":3
}
"X 0 Z" {
    >Set:
        "Enable":0
        "Input":3
    >Step 10
    >Check:
        "Output":2
}
"X 1 X" {
    >Set:
        "Enable":1
        "Input":3
    >Step 10
    >Check:
        "Output":3
}
"X Z X" {
    >Set:
        "Enable":2
        "Input":3
    >Step 10
    >Check:
        "Output":3
}
"X X X" {
    >Set:
        "Enable":3
        "Input":3
    >Step 10
    >Check:
        "Output":3
}