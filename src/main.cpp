#include <meta>

struct Smoke { int a; double b; };

int main() {
    template for (constexpr auto m : std::define_static_array(
            std::meta::nonstatic_data_members_of(^^Smoke,
                std::meta::access_context::unchecked()))) {
        (void) std::meta::identifier_of(m);
    }
}