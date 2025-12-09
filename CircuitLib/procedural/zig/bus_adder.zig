const cm = @import("cm.zig");

const UUID_: [*:0]const u8 = "f44e3c46-9e2f-42e0-8c19-80d12e4b12da";
const Name_: [*:0]const u8 = "Bus Adder";
const DefaultParams_: [*:0]const u8 = "(\"size\": 1)";

export fn getUUID() [*:0]const u8 {
    return UUID_;
}
export fn getName() [*:0]const u8 {
    return Name_;
}
export fn getDefaultParameters() [*:0]const u8 {
    return DefaultParams_;
}

fn make_full_adder(
    x: cm.coord_t,
    y: cm.coord_t,
    input_a: cm.ConnectionPoint,
    input_b: cm.ConnectionPoint,
    carry_in: cm.ConnectionPoint,
    sum_out: cm.ConnectionPoint,
) cm.ConnectionPoint {
    const AND = cm.getPrimitiveType("AND");
    const OR = cm.getPrimitiveType("OR");
    const XOR = cm.getPrimitiveType("XOR");
    const root_xor = cm.createBlockAtPosition(x, y, 0, XOR);
    const root_and = cm.createBlockAtPosition(x + 1, y, 0, AND);
    const root_or = cm.createBlockAtPosition(x + 2, y, 0, OR);
    const carry_xor = cm.createBlockAtPosition(x + 3, y, 0, XOR);
    const carry_xor_input = cm.ConnectionPoint.init(carry_xor, 0);
    const carry_xor_output = cm.ConnectionPoint.init(carry_xor, 1);
    const roots = [_]i32{ root_xor, root_and, root_or };
    const inputs = [_]cm.ConnectionPoint{
        input_a,
        input_b,
        carry_in,
    };
    inline for (roots) |i| {
        inline for (inputs) |j| {
            j.connectTo(cm.ConnectionPoint.init(i, 0));
        }
        cm.ConnectionPoint.init(i, 1).connectTo(carry_xor_input);
    }
    cm.ConnectionPoint.init(root_xor, 1).connectTo(sum_out);
    return carry_xor_output;
}

export fn generateCircuit() bool {
    const bit_width = cm.getParameter("size");
    if (bit_width <= 0) {
        cm.logError("Bus Adder: 'size' parameter must be greater than 0");
        return false;
    }
    const BUS = cm.getBusBlock(bit_width);
    cm.setSize(5, 3);
    const SWITCH = cm.getPrimitiveType("SWITCH");
    const LIGHT = cm.getPrimitiveType("LIGHT");

    const carry_in = cm.createBlockAtPosition(0, 0, 0, SWITCH);
    const carry_out = cm.createBlockAtPosition(7, bit_width, 0, LIGHT);
    const bus_a = cm.createBlockAtPosition(0, 1, 6, BUS);
    const bus_b = cm.createBlockAtPosition(0, bit_width + 1, 6, BUS);
    const bus_out = cm.createBlockAtPosition(6, 0, 0, BUS);

    const carry_in_cp = cm.ConnectionPoint.init(carry_in, 0);
    const carry_out_cp = cm.ConnectionPoint.init(carry_out, 0);
    const bus_a_cp = cm.ConnectionPoint.init(bus_a, @intCast(bit_width));
    const bus_b_cp = cm.ConnectionPoint.init(bus_b, @intCast(bit_width));
    const bus_out_cp = cm.ConnectionPoint.init(bus_out, @intCast(bit_width));

    var previous_carry = carry_in_cp;
    for (0..@intCast(bit_width)) |i_| {
        const i: i32 = @intCast(i_);
        const input_a = cm.ConnectionPoint.init(bus_a, i);
        const input_b = cm.ConnectionPoint.init(bus_b, i);
        const sum_out = cm.ConnectionPoint.init(bus_out, i);
        previous_carry = make_full_adder(
            2,
            i,
            input_a,
            input_b,
            previous_carry,
            sum_out,
        );
    }
    previous_carry.connectTo(carry_out_cp);
    _ = carry_in_cp.addAsInputNamed(0, 0, "c");
    _ = carry_out_cp.addAsOutputNamed(4, 2, "c");
    cm.setConnectionPortBitWidth(bus_a_cp.addAsInputNamed(0, 1, "a"), @intCast(bit_width));
    cm.setConnectionPortBitWidth(bus_b_cp.addAsInputNamed(0, 2, "b"), @intCast(bit_width));
    cm.setConnectionPortBitWidth(bus_out_cp.addAsOutputNamed(4, 1, "s"), @intCast(bit_width));

    return true;
}
