#!/bin/bash

echo "=== ESP32 Gong/Ring Project Build Check ==="
echo

# Check project structure
echo "1. Project Structure:"
if [ -d "src" ] && [ -d "include" ] && [ -d "data" ]; then
    echo "   ✓ src/ directory exists"
    echo "   ✓ include/ directory exists"
    echo "   ✓ data/ directory exists"
else
    echo "   ✗ Missing required directories"
fi

# Check source files
echo
echo "2. Source Files:"
src_files=("main.cpp" "webhandler.cpp" "lorahandler.cpp" "mp3handler.cpp" "schedule.cpp")
for file in "${src_files[@]}"; do
    if [ -f "src/$file" ]; then
        echo "   ✓ $file"
    else
        echo "   ✗ Missing $file"
    fi
done

# Check header files
echo
echo "3. Header Files:"
header_files=("webhandler.h" "lorahandler.h" "mp3handler.h" "schedule.h")
for file in "${header_files[@]}"; do
    if [ -f "include/$file" ]; then
        echo "   ✓ $file"
    else
        echo "   ✗ Missing $file"
    fi
done

# Check data files
echo
echo "4. Data Files:"
if [ -f "data/index.html" ]; then
    echo "   ✓ index.html"
else
    echo "   ✗ Missing index.html"
fi

# Check configuration files
echo
echo "5. Configuration Files:"
if [ -f "platformio.ini" ]; then
    echo "   ✓ platformio.ini"
else
    echo "   ✗ Missing platformio.ini"
fi

if [ -f "README.md" ]; then
    echo "   ✓ README.md"
else
    echo "   ✗ Missing README.md"
fi

# Check for common syntax issues
echo
echo "6. Code Quality Checks:"
echo "   Checking for balanced braces..."
open_braces=$(grep -o "{" src/*.cpp | wc -l)
close_braces=$(grep -o "}" src/*.cpp | wc -l)
if [ "$open_braces" -eq "$close_braces" ]; then
    echo "   ✓ Braces are balanced ($open_braces open, $close_braces close)"
else
    echo "   ✗ Brace mismatch: $open_braces open, $close_braces close"
fi

echo "   Checking for semicolon issues..."
if grep -q ";;" src/*.cpp; then
    echo "   ✗ Found double semicolons"
else
    echo "   ✓ No double semicolons found"
fi

echo "   Checking for missing includes..."
missing_includes=0
for cpp_file in src/*.cpp; do
    base_name=$(basename "$cpp_file" .cpp)
    if [ ! -f "include/${base_name}.h" ]; then
        echo "   ✗ Missing header for $cpp_file"
        missing_includes=$((missing_includes + 1))
    fi
done
if [ $missing_includes -eq 0 ]; then
    echo "   ✓ All source files have corresponding headers"
fi

echo
echo "7. Next Steps:"
echo "   To build this project:"
echo "   1. Install PlatformIO: pip install platformio"
echo "   2. Run: platformio run"
echo "   3. Upload: platformio run --target upload"
echo "   4. Monitor: platformio device monitor"
echo
echo "   To configure WiFi:"
echo "   Edit src/webhandler.cpp and set your WiFi credentials"
echo
echo "=== Build Check Complete ==="
