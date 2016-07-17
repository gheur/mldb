/* tokenize.h                                        -*- C++ -*-
   Mathieu Marquis Bolduc, October 5th 2015
   Copyright (c) 2015 Datacratic.  All rights reserved.

   This file is part of MLDB. Copyright 2015 Datacratic. All rights reserved.

   Generic delimiter-token parsing.
*/

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include "types/string.h"
#include "mldb/types/value_description_fwd.h"
#include "cell_value.h"

namespace ML {
struct Parse_Context;
}

namespace Datacratic {

/** Common options for the tokenize function. */
struct TokenizeOptions {
    Utf8String splitchar = ",";
    Utf8String quotechar = "";
    int offset = 0, limit = -1;
    MLDB::CellValue value;
    int min_token_length = 1;
    std::pair<int, int> ngram_range = { 1, 1};
};

/** Allow these options to be accessed and documented via the ValueDescription
    system.
*/
DECLARE_STRUCTURE_DESCRIPTION(TokenizeOptions);

void
tokenize_exec(std::function<bool (Utf8String&)> exec,
              ML::Parse_Context& context,
              const Utf8String& splitchars,
              const Utf8String& quotechar,
              int min_token_length);

char32_t expectUtf8Char(ML::Parse_Context & context);

bool tokenize(std::unordered_map<Utf8String, int>& bagOfWords,
              ML::Parse_Context& pcontext,
              const TokenizeOptions & options);

Utf8String token_extract(ML::Parse_Context& context,
                         int nth,
                         const TokenizeOptions & options);

} // namespace Datacratic

