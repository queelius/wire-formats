# PFC Documentation Guide

This document describes the documentation infrastructure for the PFC (Prefix-Free Codecs) library.

## Documentation Architecture

The PFC library uses a multi-layered documentation approach:

### 1. MkDocs (User-Facing Documentation)

**Purpose**: High-level guides, tutorials, and concept explanations

**Location**: `docs/` directory

**Build Tool**: MkDocs with Material theme

**Deployment**: GitHub Pages at https://queelius.github.io/pfc/

**Configuration**: `mkdocs.yml`

#### Structure

```
docs/
├── index.md                    # Landing page
├── api/                        # API reference (hand-written)
│   ├── overview.md
│   ├── core.md
│   ├── codecs.md
│   ├── succinct.md            # NEW: Succinct structures
│   └── ...
├── guides/                     # Tutorials and guides
│   ├── installation.md
│   ├── quick-start.md
│   ├── design-principles.md
│   └── ...
└── examples/                   # Usage examples
    ├── basic.md
    ├── codecs.md
    └── ...
```

### 2. Doxygen (API Reference)

**Purpose**: Automated API documentation from source code

**Location**: Generated in `docs/api-reference/html/`

**Build Tool**: Doxygen

**Configuration**: `Doxyfile` (root directory)

**Features**:
- Class/struct documentation
- Function signatures and parameters
- Inheritance diagrams
- Include dependency graphs
- Source code browsing
- Cross-references

### 3. Inline Documentation

**Purpose**: Code-level documentation for developers

**Location**: Header files in `include/pfc/`

**Style**: C++ comments with Doxygen-compatible formatting

**Standards**:
- All public APIs documented
- Concepts explained with requirements
- Examples in code comments
- Design rationale preserved

## Building Documentation

### MkDocs Documentation

#### Prerequisites

```bash
# Install MkDocs and Material theme
pip install mkdocs mkdocs-material

# Or use the requirements.txt
pip install -r requirements.txt
```

#### Build and Serve Locally

```bash
# Serve with live reload (for development)
mkdocs serve

# View at http://127.0.0.1:8000/

# Build static site
mkdocs build

# Output in site/ directory
```

#### Deploy to GitHub Pages

```bash
# Deploy to gh-pages branch
mkdocs gh-deploy

# This will:
# 1. Build the documentation
# 2. Push to gh-pages branch
# 3. GitHub Pages will serve it automatically
```

### Doxygen Documentation

#### Prerequisites

```bash
# Install Doxygen
sudo apt-get install doxygen graphviz  # Ubuntu/Debian
brew install doxygen graphviz          # macOS
```

#### Generate API Reference

```bash
# Generate Doxygen documentation
doxygen Doxyfile

# Output in docs/api-reference/html/

# View locally
open docs/api-reference/html/index.html  # macOS
xdg-open docs/api-reference/html/index.html  # Linux
```

## Documentation Standards

### MkDocs Pages

**Format**: Markdown with extensions

**Guidelines**:
- Use clear headings (# for h1, ## for h2, etc.)
- Include code examples with syntax highlighting
- Add admonitions for warnings/notes
- Cross-reference related pages
- Keep pages focused (one topic per page)

**Example**:

````markdown
# Feature Name

Brief description of the feature.

## Basic Usage

```cpp
#include <pfc/feature.hpp>
using namespace pfc;

// Example code
auto result = feature_function(data);
```

!!! note
    This feature requires C++20.

## Advanced Usage

For advanced use cases, see [Advanced Guide](../guides/advanced.md).
````

### Inline Code Documentation

**Format**: C++ comments (Doxygen-compatible)

**Guidelines**:

```cpp
/**
 * @brief Brief one-line description
 *
 * Detailed description of the class/function.
 * Can span multiple lines and include:
 * - Implementation details
 * - Design rationale
 * - Performance characteristics
 *
 * @tparam T Template parameter description
 * @param value Parameter description
 * @return Description of return value
 *
 * @code
 * // Example usage
 * MyClass obj;
 * obj.method(42);
 * @endcode
 *
 * @note Additional notes
 * @warning Important warnings
 * @see RelatedClass
 */
template<typename T>
class MyClass {
    // ...
};
```

### API Reference Pages (MkDocs)

**Format**: Structured Markdown

**Template**:

```markdown
# Component Name

Brief overview paragraph.

## Overview

Detailed description of the component.

## Key Classes/Functions

### ClassName

Description of the class.

#### Construction

```cpp
ClassName(param1, param2);
```

#### Methods

```cpp
ReturnType method_name(ParamType param);
```

Description of what the method does.

## Examples

### Basic Example

```cpp
// Code example
```

### Advanced Example

```cpp
// More complex example
```

## Performance

Time/space complexity analysis.

## See Also

- [Related Component](other.md)
```

## Documentation Workflows

### Adding New Features

When adding a new feature to PFC:

1. **Write inline documentation** in the header file
   - Document all public APIs
   - Include usage examples
   - Explain design decisions

2. **Create/update MkDocs page** in `docs/api/`
   - High-level overview
   - Usage examples
   - Best practices

3. **Update navigation** in `mkdocs.yml`
   - Add page to appropriate section

4. **Add examples** if appropriate
   - Create example file in `examples/`
   - Reference in documentation

5. **Update main README** if it's a major feature
   - Add to feature list
   - Update quick start if relevant

### Updating Existing Documentation

1. **Identify outdated content**
   - Check inline comments
   - Review MkDocs pages
   - Verify examples still work

2. **Update all layers**
   - Inline documentation
   - MkDocs pages
   - Examples
   - README if necessary

3. **Rebuild and verify**
   ```bash
   mkdocs serve   # Check MkDocs
   doxygen        # Regenerate API ref
   ```

### Documentation Review Checklist

Before releasing new documentation:

- [ ] All new APIs have inline documentation
- [ ] MkDocs pages are complete and accurate
- [ ] Code examples compile and run
- [ ] Cross-references are correct
- [ ] Spelling and grammar checked
- [ ] Navigation structure is logical
- [ ] Search functionality works
- [ ] Mobile view is readable
- [ ] Performance notes are accurate
- [ ] Known limitations documented

## GitHub Pages Deployment

### Automatic Deployment (Recommended)

Create `.github/workflows/docs.yml`:

```yaml
name: Deploy Documentation

on:
  push:
    branches:
      - main
  workflow_dispatch:

permissions:
  contents: write

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Configure Git Credentials
        run: |
          git config user.name github-actions[bot]
          git config user.email 41898282+github-actions[bot]@users.noreply.github.com

      - uses: actions/setup-python@v4
        with:
          python-version: 3.x

      - run: pip install mkdocs-material

      - name: Deploy MkDocs
        run: mkdocs gh-deploy --force
```

This will automatically deploy documentation when you push to main.

### Manual Deployment

```bash
# Ensure you're on main branch
git checkout main

# Deploy MkDocs
mkdocs gh-deploy

# Optionally, also commit Doxygen output
doxygen Doxyfile
git add docs/api-reference/html
git commit -m "Update API reference"
git push
```

### GitHub Pages Configuration

1. Go to repository Settings → Pages
2. Source: Deploy from branch
3. Branch: `gh-pages` / `root`
4. Save

Documentation will be available at:
- https://queelius.github.io/pfc/ (MkDocs)
- https://queelius.github.io/pfc/api-reference/html/ (Doxygen)

## Documentation Maintenance

### Regular Tasks

**Weekly**:
- Review and respond to documentation issues
- Fix typos and broken links
- Update examples if APIs change

**Monthly**:
- Review test coverage and update docs accordingly
- Check for outdated performance claims
- Update benchmarks if significant

**Per Release**:
- Update version numbers
- Update feature lists
- Review all getting started guides
- Verify all examples still work
- Update changelog/release notes

### Documentation Metrics

Track these metrics to ensure documentation quality:

- **Coverage**: % of public APIs documented
- **Freshness**: Time since last update
- **Accuracy**: Example code compile rate
- **Usability**: User feedback/issues
- **Completeness**: All sections filled in

### Tools for Quality

```bash
# Check for broken links in MkDocs
mkdocs build --strict

# Spell check
aspell check docs/**/*.md

# Validate code examples (compile all examples)
cd build && make examples && ./run_all_examples.sh

# Check Doxygen warnings
doxygen Doxyfile 2>&1 | grep warning
```

## Best Practices

### Writing Style

- **Be concise**: Get to the point quickly
- **Be precise**: Use exact terminology
- **Be practical**: Include real examples
- **Be complete**: Cover edge cases
- **Be consistent**: Follow established patterns

### Code Examples

- **Minimal**: Show only relevant code
- **Complete**: Include necessary headers
- **Compilable**: Verify examples work
- **Practical**: Demonstrate real use cases
- **Progressive**: Start simple, add complexity

### Organization

- **Logical structure**: Related content together
- **Progressive disclosure**: Overview → details
- **Clear navigation**: Easy to find information
- **Cross-referencing**: Link related topics
- **Search optimization**: Use clear headings

## Resources

### MkDocs
- [MkDocs Documentation](https://www.mkdocs.org/)
- [Material Theme](https://squidfunk.github.io/mkdocs-material/)
- [Markdown Extensions](https://python-markdown.github.io/extensions/)

### Doxygen
- [Doxygen Manual](https://www.doxygen.nl/manual/)
- [Doxygen Comments](https://www.doxygen.nl/manual/docblocks.html)
- [Doxygen Commands](https://www.doxygen.nl/manual/commands.html)

### Writing
- [Google Developer Documentation Style Guide](https://developers.google.com/style)
- [Microsoft Writing Style Guide](https://docs.microsoft.com/en-us/style-guide/)
- [Write the Docs](https://www.writethedocs.org/)

## Support

For documentation questions or issues:

1. Check this guide first
2. Search existing issues
3. Open a new issue with label `documentation`
4. For major changes, open a discussion first

## Current Status

### Completed
- [x] MkDocs infrastructure set up
- [x] Material theme configured
- [x] Basic API documentation structure
- [x] Doxygen configuration created
- [x] README updated with new features
- [x] SuccinctBitVector documentation added
- [x] Range coding limitations documented
- [x] Test coverage status updated

### In Progress
- [ ] Comprehensive API reference for all components
- [ ] More usage examples
- [ ] Video tutorials
- [ ] Interactive code playgrounds

### Planned
- [ ] Automatic example validation in CI
- [ ] Documentation coverage reporting
- [ ] Interactive API explorer
- [ ] Multi-version documentation
- [ ] PDF export option
- [ ] Searchable code examples

## Version History

- **1.0.0** (2025-11-17): Initial documentation infrastructure
  - MkDocs with Material theme
  - Doxygen configuration
  - SuccinctBitVector documentation
  - Updated README and badges
