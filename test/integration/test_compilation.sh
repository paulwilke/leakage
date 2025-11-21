#!/bin/bash

# ============================================================================
# ESPHome UI Framework - Integration Test: Compilation Validation
# ============================================================================
#
# This script validates that all example configurations compile successfully
# with ESPHome. It checks:
#
# 1. YAML syntax validity
# 2. Include file resolution
# 3. ESPHome compilation (without uploading)
# 4. Memory usage validation
# 5. Binary size checks
#
# Usage:
#   ./test_compilation.sh [--verbose] [--example=<name>]
#
# Options:
#   --verbose       Show detailed compilation output
#   --example=NAME  Test only specific example (bme280, ds18b20, relay, demo)
#   --quick         Skip full compilation, only validate YAML
#
# Exit codes:
#   0 = All tests passed
#   1 = Compilation failed
#   2 = YAML validation failed
#   3 = Memory usage exceeded limits
#   4 = Prerequisites missing
# ============================================================================

set -e  # Exit on error

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
EXAMPLES_DIR="$PROJECT_ROOT/examples"
FRAMEWORK_DIR="$PROJECT_ROOT/framework"
TEST_OUTPUT_DIR="$SCRIPT_DIR/output"

# Test configuration
MAX_BINARY_SIZE_KB=1024  # 1MB max
MAX_RAM_USAGE_PERCENT=80  # 80% of ESP32 RAM
VERBOSE=false
QUICK_MODE=false
SPECIFIC_EXAMPLE=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ============================================================================
# Helper Functions
# ============================================================================

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
}

log_section() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

# ============================================================================
# Prerequisites Check
# ============================================================================

check_prerequisites() {
    log_section "Checking Prerequisites"

    # Check for ESPHome
    if ! command -v esphome &> /dev/null; then
        log_error "ESPHome not found. Please install: pip install esphome"
        exit 4
    fi

    local esphome_version=$(esphome version | head -n1)
    log_success "ESPHome found: $esphome_version"

    # Check for Python
    if ! command -v python3 &> /dev/null; then
        log_error "Python 3 not found"
        exit 4
    fi

    log_success "Python 3 found: $(python3 --version)"

    # Check framework files exist
    if [ ! -d "$FRAMEWORK_DIR" ]; then
        log_error "Framework directory not found: $FRAMEWORK_DIR"
        exit 4
    fi

    log_success "Framework directory found"

    # Check examples directory
    if [ ! -d "$EXAMPLES_DIR" ]; then
        log_error "Examples directory not found: $EXAMPLES_DIR"
        exit 4
    fi

    log_success "Examples directory found"

    # Create output directory
    mkdir -p "$TEST_OUTPUT_DIR"
    log_success "Test output directory: $TEST_OUTPUT_DIR"

    echo ""
}

# ============================================================================
# YAML Validation
# ============================================================================

validate_yaml() {
    local yaml_file=$1
    local example_name=$(basename $(dirname "$yaml_file"))

    log_info "Validating YAML: $example_name"

    # Check if file exists
    if [ ! -f "$yaml_file" ]; then
        log_error "YAML file not found: $yaml_file"
        return 1
    fi

    # Check basic YAML syntax with Python
    if ! python3 -c "import yaml; yaml.safe_load(open('$yaml_file'))" 2>/dev/null; then
        log_error "YAML syntax error in: $yaml_file"
        return 1
    fi

    log_success "YAML syntax valid: $example_name"

    # Check for required sections
    local required_sections=("esphome" "esp32" "wifi")
    for section in "${required_sections[@]}"; do
        if ! grep -q "^$section:" "$yaml_file"; then
            log_warning "Missing section '$section' in: $yaml_file"
        fi
    done

    # Check for secrets usage
    if grep -q "!secret" "$yaml_file"; then
        log_info "  Uses secrets.yaml (OK)"
    fi

    # Check for framework includes
    if grep -q "framework/include" "$yaml_file"; then
        log_success "  Framework includes found"
    else
        log_warning "  No framework includes found"
    fi

    return 0
}

# ============================================================================
# ESPHome Compilation
# ============================================================================

compile_example() {
    local yaml_file=$1
    local example_name=$(basename $(dirname "$yaml_file"))
    local example_dir=$(dirname "$yaml_file")
    local output_file="$TEST_OUTPUT_DIR/${example_name}.log"

    log_section "Compiling: $example_name"

    # Create secrets.yaml if it doesn't exist
    local secrets_file="$example_dir/secrets.yaml"
    if [ ! -f "$secrets_file" ]; then
        log_info "Creating temporary secrets.yaml"
        cat > "$secrets_file" << EOF
# Temporary secrets for testing
wifi_ssid: "TestNetwork"
wifi_password: "TestPassword123"
api_key: "$(python3 -c 'import secrets; print(secrets.token_urlsafe(32))')"
ota_password: "test_ota_password"
EOF
    fi

    # Run ESPHome compile
    local start_time=$(date +%s)

    if [ "$VERBOSE" = true ]; then
        esphome compile "$yaml_file" 2>&1 | tee "$output_file"
        local compile_result=${PIPESTATUS[0]}
    else
        esphome compile "$yaml_file" > "$output_file" 2>&1
        local compile_result=$?
    fi

    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    # Check compilation result
    if [ $compile_result -ne 0 ]; then
        log_error "Compilation failed for: $example_name (${duration}s)"
        log_error "See log: $output_file"

        # Show last 20 lines of error
        if [ "$VERBOSE" = false ]; then
            echo ""
            log_info "Last 20 lines of output:"
            tail -n 20 "$output_file"
        fi

        return 1
    fi

    log_success "Compilation successful: $example_name (${duration}s)"

    # Analyze compilation output
    analyze_compilation_output "$output_file" "$example_name"

    return 0
}

# ============================================================================
# Compilation Output Analysis
# ============================================================================

analyze_compilation_output() {
    local log_file=$1
    local example_name=$2

    log_info "Analyzing build output..."

    # Extract memory usage
    if grep -q "RAM:" "$log_file"; then
        local ram_line=$(grep "RAM:" "$log_file" | tail -n1)
        log_info "  $ram_line"

        # Check if RAM usage exceeds limit
        local ram_percent=$(echo "$ram_line" | grep -oP '\d+\.\d+%' | head -n1 | tr -d '%')
        if (( $(echo "$ram_percent > $MAX_RAM_USAGE_PERCENT" | bc -l) )); then
            log_warning "  RAM usage high: ${ram_percent}% (limit: ${MAX_RAM_USAGE_PERCENT}%)"
        fi
    fi

    # Extract Flash usage
    if grep -q "Flash:" "$log_file"; then
        local flash_line=$(grep "Flash:" "$log_file" | tail -n1)
        log_info "  $flash_line"
    fi

    # Check for warnings
    local warning_count=$(grep -c "warning:" "$log_file" || true)
    if [ $warning_count -gt 0 ]; then
        log_warning "  $warning_count compiler warnings found"
    else
        log_success "  No compiler warnings"
    fi

    # Check binary size
    local elf_pattern=".esphome/build/${example_name}/.pioenvs/${example_name}/firmware.elf"
    if [ -f "$elf_pattern" ]; then
        local size_kb=$(du -k "$elf_pattern" | cut -f1)
        log_info "  Binary size: ${size_kb}KB"

        if [ $size_kb -gt $MAX_BINARY_SIZE_KB ]; then
            log_warning "  Binary size exceeds limit: ${size_kb}KB > ${MAX_BINARY_SIZE_KB}KB"
        fi
    fi

    echo ""
}

# ============================================================================
# Include File Validation
# ============================================================================

validate_includes() {
    local yaml_file=$1
    local example_dir=$(dirname "$yaml_file")

    log_info "Validating include files..."

    # Extract includes from YAML
    local includes=$(grep "includes:" -A 10 "$yaml_file" | grep "^    - " | sed 's/^    - //')

    local all_found=true
    for include in $includes; do
        # Resolve relative path
        local include_path="$example_dir/$include"

        if [ -f "$include_path" ]; then
            log_success "  Found: $include"
        else
            log_error "  Missing: $include"
            all_found=false
        fi
    done

    if [ "$all_found" = true ]; then
        log_success "All include files found"
    else
        log_error "Some include files missing"
        return 1
    fi

    return 0
}

# ============================================================================
# Test Runner
# ============================================================================

run_tests() {
    local examples_to_test=()

    # Determine which examples to test
    if [ -n "$SPECIFIC_EXAMPLE" ]; then
        examples_to_test=("$SPECIFIC_EXAMPLE")
    else
        # Test all examples
        examples_to_test=("bme280" "ds18b20" "relay" "demo")
    fi

    local total_tests=0
    local passed_tests=0
    local failed_tests=0

    for example in "${examples_to_test[@]}"; do
        local yaml_file=""

        # Find YAML file for example
        case "$example" in
            "bme280")
                yaml_file="$EXAMPLES_DIR/bme280/bme280_example.yaml"
                ;;
            "ds18b20")
                yaml_file="$EXAMPLES_DIR/ds18b20/ds18b20_example.yaml"
                ;;
            "relay")
                yaml_file="$EXAMPLES_DIR/relay/relay_example.yaml"
                ;;
            "demo")
                yaml_file="$EXAMPLES_DIR/demo/demo.yaml"
                ;;
            *)
                log_error "Unknown example: $example"
                continue
                ;;
        esac

        if [ ! -f "$yaml_file" ]; then
            log_warning "YAML file not found for $example, skipping"
            continue
        fi

        total_tests=$((total_tests + 1))

        # Run tests for this example
        local example_passed=true

        # YAML validation
        if ! validate_yaml "$yaml_file"; then
            example_passed=false
        fi

        # Include validation
        if ! validate_includes "$yaml_file"; then
            example_passed=false
        fi

        # Compilation (unless quick mode)
        if [ "$QUICK_MODE" = false ]; then
            if ! compile_example "$yaml_file"; then
                example_passed=false
            fi
        else
            log_info "Skipping compilation (quick mode)"
        fi

        # Update counters
        if [ "$example_passed" = true ]; then
            passed_tests=$((passed_tests + 1))
            log_success "All tests passed for: $example"
        else
            failed_tests=$((failed_tests + 1))
            log_error "Some tests failed for: $example"
        fi

        echo ""
    done

    # Print summary
    log_section "Test Summary"
    echo "Total examples tested: $total_tests"
    echo -e "${GREEN}Passed: $passed_tests${NC}"
    echo -e "${RED}Failed: $failed_tests${NC}"
    echo ""

    if [ $failed_tests -eq 0 ]; then
        log_success "All integration tests passed!"
        return 0
    else
        log_error "$failed_tests example(s) failed"
        return 1
    fi
}

# ============================================================================
# Cleanup
# ============================================================================

cleanup() {
    log_info "Cleaning up temporary files..."

    # Remove temporary secrets files
    find "$EXAMPLES_DIR" -name "secrets.yaml" -type f -exec grep -q "Temporary secrets for testing" {} \; -delete 2>/dev/null || true

    log_success "Cleanup complete"
}

# ============================================================================
# Main
# ============================================================================

parse_args() {
    for arg in "$@"; do
        case $arg in
            --verbose)
                VERBOSE=true
                ;;
            --quick)
                QUICK_MODE=true
                ;;
            --example=*)
                SPECIFIC_EXAMPLE="${arg#*=}"
                ;;
            --help)
                head -n 30 "$0" | tail -n +2 | grep "^#" | sed 's/^# //'
                exit 0
                ;;
            *)
                log_error "Unknown argument: $arg"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
}

main() {
    log_section "ESPHome UI Framework - Integration Tests"
    echo "Project: $PROJECT_ROOT"
    echo "Test output: $TEST_OUTPUT_DIR"
    echo ""

    parse_args "$@"

    check_prerequisites

    if run_tests; then
        cleanup
        exit 0
    else
        cleanup
        exit 1
    fi
}

# Run main function
main "$@"
