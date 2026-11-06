#define RETAIL_REGISTER(Name, Expression) bool RetailTest_##Name() { return Expression; }

#ifndef RETAIL_REGISTER_math
#define RETAIL_REGISTER_math(Name, Expression)
#endif
#ifndef RETAIL_REGISTER_geometry
#define RETAIL_REGISTER_geometry(Name, Expression)
#endif
#ifndef RETAIL_REGISTER_dynamics
#define RETAIL_REGISTER_dynamics(Name, Expression)
#endif
#ifndef RETAIL_REGISTER_contact
#define RETAIL_REGISTER_contact(Name, Expression)
#endif
#ifndef RETAIL_REGISTER_pressure
#define RETAIL_REGISTER_pressure(Name, Expression)
#endif
#ifndef RETAIL_REGISTER_continuous
#define RETAIL_REGISTER_continuous(Name, Expression)
#endif

#define RETAIL_SCENARIO(Name, Family, Expression) RETAIL_REGISTER_##Family(Name, Expression)
#include "retail_scenarios.inc"
#undef RETAIL_SCENARIO
#undef RETAIL_REGISTER_math
#undef RETAIL_REGISTER_geometry
#undef RETAIL_REGISTER_dynamics
#undef RETAIL_REGISTER_contact
#undef RETAIL_REGISTER_pressure
#undef RETAIL_REGISTER_continuous
#undef RETAIL_REGISTER
