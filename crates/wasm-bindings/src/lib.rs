use compiler_lab::compiler_trace;
use emu_core::{cpu_trace, riscv_trace};
use hypervisor_lab::virtualization_trace;
use os_lab::os_trace;
use wasm_bindgen::prelude::*;

#[wasm_bindgen]
pub fn run_scenario(scenario: &str, source: &str) -> Result<String, JsError> {
    let trace = match scenario {
        "riscv" => riscv_trace(source).map_err(|error| JsError::new(&error))?,
        "cpu" => cpu_trace(source),
        "os" => os_trace(source),
        "virt" => virtualization_trace(source),
        "compiler" => compiler_trace(source),
        _ => {
            return Err(JsError::new(
                "unknown scenario: expected one of riscv, cpu, os, virt, compiler",
            ));
        }
    };

    serde_json::to_string(&trace).map_err(|error| JsError::new(&error.to_string()))
}
