struct MainController {}

struct Controller {
    name: String,
}

fn main() {
    // Main Controller
    //   - Needs to know all the controllers it can communicate with

    // Controller
    //   - Needs to know its inputs
    //   - Needs to know its outputs
    //   - CAN Bus IDs

    let back_controller = Controller {
        name: "Back Controller".to_string(),
    };
}
