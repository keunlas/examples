// include "jwt-cpp/traits/author-library/traits.h"
#include "jwt-cpp/traits/nlohmann-json/traits.h"
// There is also a "defaults.h" if you's like to skip providing the
// template specializations for the JSON traits

int main() {
  // All the provided traits are in jwt::traits namespace
  using traits = jwt::traits::nlohmann_json;

  const auto time = jwt::date::clock::now();
  const auto token = jwt::create<traits>()
                         .set_type("JWT")
                         .set_issuer("auth.mydomain.io")
                         .set_audience("mydomain.io")
                         .set_issued_at(time)
                         .set_not_before(time)
                         .set_expires_at(time + std::chrono::minutes{2} +
                                         std::chrono::seconds{15})
                         //  .sign(jwt::algorithm::none{});
                         .sign(jwt::algorithm::hs256("secret"));

  std::cout << token << '\n';

  const auto decoded = jwt::decode<traits>(token);

  for (auto& e : decoded.get_payload_json())
    std::cout << e.first << " = " << e.second << '\n';

  try {
    jwt::verify<traits>()
        // .allow_algorithm(jwt::algorithm::none{})
        .allow_algorithm(jwt::algorithm::hs256("secret"))
        .with_issuer("auth.mydomain.io")
        .with_audience("mydomain.io")
        .verify(decoded);
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }
}
