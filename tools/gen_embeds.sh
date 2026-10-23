if [ ! -s "out.tbl" ]; then
    echo "Error: out.tbl not found. Generate using parser generator first."
    exit 1
fi

echo "Generating embedded parser tables..."

# Generate the embedded parser tables                  

cat <<EOF > src/embeds.hpp
#ifndef EMBEDS_HPP
#define EMBEDS_HPP
inline const char* JOOS_GRAMMAR = R"(
$(cat tools/grammar.cfg)
)";

inline const char* JOOS_PARSER_TABLES = R"(
$(cat out.tbl)
)";

#endif // EMBEDS_HPP
EOF
echo "Embedded parser tables generated in src/embeds.hpp"