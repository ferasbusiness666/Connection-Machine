use std::ffi::CString;
use std::ptr;

const UUID_: &str = "abd7f5c1-2e89-4630-bf12-d596c812dde8";
const NAME_: &str = "Edge Detector";
const DEFAULT_PARAMS_: &str = r#"("pulse width": 3, "{1:R,2:F,3:B}": 2)"#;

// Safe wrappers for string return
#[no_mangle]
pub extern "C" fn getUUID() -> *const u8 {
    UUID_.as_ptr()
}

#[no_mangle]
pub extern "C" fn getName() -> *const u8 {
    NAME_.as_ptr()
}

#[no_mangle]
pub extern "C" fn getDefaultParameters() -> *const u8 {
    DEFAULT_PARAMS_.as_ptr()
}

#[no_mangle]
pub extern "C" fn generateCircuit() -> bool {
    unsafe {
        // Fetch parameters
        let pulse = cm::getParameter(c_str("pulse width"));
        let kind = cm::getParameter(c_str("{1:R,2:F,3:B}"));

        if pulse < 1 {
            cm::logError(c_str("Pulse width should not be less than 1."));
            return false;
        }

        let pulse = pulse + 1;

        // Get primitive block types
        let and_type = cm::getPrimitiveType(c_str("AND"));
        let nor_type = cm::getPrimitiveType(c_str("NOR"));
        let xnor_type = cm::getPrimitiveType(c_str("XNOR"));
        let switch_type = cm::getPrimitiveType(c_str("SWITCH"));
        let light_type = cm::getPrimitiveType(c_str("LIGHT"));

        cm::setSize(1, 1);

        // Create end gate based on kind
        let end_gate = match kind {
            1 => cm::createBlockAtPosition(pulse, 0, 0, and_type),
            2 => cm::createBlockAtPosition(pulse, 0, 0, nor_type),
            3 => cm::createBlockAtPosition(pulse, 0, 0, xnor_type),
            _ => {
                cm::logError(c_str("{1:R,2:F,3:B} should only be 1, 2, or 3 and nothing else!"));
                return false;
            }
        };

        // Input block
        let input = cm::createBlockAtPosition(0, 0, 0, switch_type);
        cm::addConnectionInput(0, 0, input, 0);
        cm::createConnection(input, 0, end_gate, 0);

        // Wait chain
        let mut wait = cm::createBlockAtPosition(1, 1, 0, nor_type);
        cm::createConnection(input, 0, wait, 0);

        for i in 0..pulse {
            let tmp = cm::createBlockAtPosition(1, 1 + i, 0, and_type);
            cm::createConnection(wait, 1, tmp, 0);
            wait = tmp;
        }

        cm::createConnection(wait, 1, end_gate, 0);

        // Output block
        let output = cm::createBlockAtPosition(pulse + 3, 0, 0, light_type);
        cm::createConnection(end_gate, 1, output, 0);
        cm::addConnectionOutput(0, 0, output, 0);

        true
    }
}

// Helper function for null-terminated C strings
fn c_str(s: &str) -> *const u8 {
    CString::new(s).unwrap().as_ptr()
}
