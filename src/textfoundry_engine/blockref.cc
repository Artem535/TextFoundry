//
// Created by artem.d on 28.01.2026.
//

#include "blockref.h"

namespace tf {

Result<Params> BlockRef::resolveParams(
    const Block& block,
    const Params& runtimeContext
) const {
    // Start with all parameters needed by the template
    auto paramNames = block.templ().extractParamNames();
    Params resolved;

    for (const auto& paramName : paramNames) {
        auto result = block.resolveParam(paramName, localParams_, runtimeContext);
        if (result.hasError()) {
            return result.error();
        }
        resolved[paramName] = result.value();
    }

    return resolved;
}

} // namespace tf
