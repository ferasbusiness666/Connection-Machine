pub const BlockType = u32;
pub const connection_end_id_t = i32;
pub const block_size_t = u8;
pub const block_id_t = i32;
pub const coord_t = i32;
pub const f_coord_t = f32;
pub const Orientation = u8;

pub extern "env" fn importFile(path: [*:0]const u8) u32;
pub extern "env" fn getParameter(key: [*:0]const u8) i32;
pub extern "env" fn getPrimitiveType(name: [*:0]const u8) BlockType;
pub extern "env" fn getNonPrimitiveType(uuid: [*:0]const u8) BlockType;
pub extern "env" fn getProceduralCircuitType(uuid: [*:0]const u8, params: [*:0]const u8) BlockType;
pub extern "env" fn getBusBlock(bit_width: i32) BlockType;
pub extern "env" fn getBusBlockAdvanced(num_inputs: i32, num_outputs: i32, input_lane_width: i32, output_lane_width: i32) BlockType;

pub extern "env" fn createBlock(block_type: BlockType) block_id_t;
pub extern "env" fn createBlockAtPosition(x: coord_t, y: coord_t, orientation: Orientation, blockType: BlockType) block_id_t;

pub extern "env" fn createConnection(source_block_id: block_id_t, source_port_id: connection_end_id_t, destination_source_id: block_id_t, destination_port_id: connection_end_id_t) void;
pub extern "env" fn addConnectionInput(external_port_x: coord_t, external_port_y: coord_t, internal_block_id: block_id_t, internal_port_id: connection_end_id_t) connection_end_id_t;
pub extern "env" fn addConnectionInputNamed(external_port_x: coord_t, external_port_y: coord_t, internal_block_id: block_id_t, internal_port_id: connection_end_id_t, port_name: [*:0]const u8) connection_end_id_t;
pub extern "env" fn addConnectionOutput(external_port_x: coord_t, external_port_y: coord_t, internal_block_id: block_id_t, internal_port_id: connection_end_id_t) connection_end_id_t;
pub extern "env" fn addConnectionOutputNamed(external_port_x: coord_t, external_port_y: coord_t, internal_block_id: block_id_t, internal_port_id: connection_end_id_t, port_name: [*:0]const u8) connection_end_id_t;
pub extern "env" fn setConnectionPortBitWidth(connection_end_id: connection_end_id_t, bit_width: u32) void;
pub extern "env" fn setConnectionPortOffset(connection_end_id: connection_end_id_t, xOffset: f_coord_t, yOffset: f_coord_t) void;

pub extern "env" fn setSize(width: coord_t, height: coord_t) void;
pub extern "env" fn logInfo(msg: [*:0]const u8) void;
pub extern "env" fn logError(msg: [*:0]const u8) void;

pub const ConnectionPoint = struct {
    block_id: block_id_t,
    port_id: connection_end_id_t,

    pub fn init(block_id: block_id_t, port_id: connection_end_id_t) ConnectionPoint {
        return ConnectionPoint{
            .block_id = block_id,
            .port_id = port_id,
        };
    }

    pub fn connectTo(self: ConnectionPoint, other: ConnectionPoint) void {
        createConnection(self.block_id, self.port_id, other.block_id, other.port_id);
    }

    pub fn addAsInput(self: ConnectionPoint, external_x: coord_t, external_y: coord_t) connection_end_id_t {
        return addConnectionInput(external_x, external_y, self.block_id, self.port_id);
    }

    pub fn addAsInputNamed(self: ConnectionPoint, external_x: coord_t, external_y: coord_t, port_name: [*:0]const u8) connection_end_id_t {
        return addConnectionInputNamed(external_x, external_y, self.block_id, self.port_id, port_name);
    }

    pub fn addAsOutput(self: ConnectionPoint, external_x: coord_t, external_y: coord_t) connection_end_id_t {
        return addConnectionOutput(external_x, external_y, self.block_id, self.port_id);
    }

    pub fn addAsOutputNamed(self: ConnectionPoint, external_x: coord_t, external_y: coord_t, port_name: [*:0]const u8) connection_end_id_t {
        return addConnectionOutputNamed(external_x, external_y, self.block_id, self.port_id, port_name);
    }
};
