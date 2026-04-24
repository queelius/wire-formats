# Algebra over Wire Formats - Pedagogical C++ Blog Posts
#
# Usage:
#   make build       - Configure and build all posts
#   make test        - Run all tests
#   make clean       - Remove build artifacts
#   make docs        - Build mkdocs site
#   make docs-serve  - Serve docs locally
#   make help        - Show this help

.PHONY: build test clean help sync docs docs-serve docs-clean

# Directory containing posts
POST_DIR := post

help:
	@echo "Algebra over Wire Formats - Pedagogical C++ Blog Posts"
	@echo ""
	@echo "Targets:"
	@echo "  make build       - Configure and build all posts"
	@echo "  make test        - Run all tests"
	@echo "  make clean       - Remove build artifacts"
	@echo "  make docs        - Build mkdocs site"
	@echo "  make docs-serve  - Serve docs locally at localhost:8000"
	@echo "  make docs-clean  - Remove docs build artifacts"
	@echo ""
	@echo "For Hugo sync, set BLOG_POST_DIR and run: make sync"

build:
	cmake -B $(POST_DIR)/build -S $(POST_DIR)
	cmake --build $(POST_DIR)/build

test: build
	ctest --test-dir $(POST_DIR)/build --output-on-failure

clean:
	rm -rf $(POST_DIR)/build

# Documentation
docs: docs-clean
	mkdir -p docs/post
	cp -r $(POST_DIR)/*-wire-formats/ docs/post/
	mkdocs build

docs-serve:
	mkdir -p docs/post
	cp -r $(POST_DIR)/*-wire-formats/ docs/post/
	mkdocs serve

docs-clean:
	rm -rf site docs/post

# Sync to external blog (requires BLOG_POST_DIR to be set).
# Per-directory rsync with --delete scoped to each post's destination.
# Unrelated content in BLOG_POST_DIR is preserved.
#
# Example:
#   BLOG_POST_DIR=~/github/repos/metafunctor/content/post make sync
sync:
ifndef BLOG_POST_DIR
	@echo "Error: BLOG_POST_DIR not set"
	@echo "Usage: BLOG_POST_DIR=/path/to/blog/post make sync"
	@exit 1
endif
	@echo "Syncing to $(BLOG_POST_DIR)..."
	@for dir in $(POST_DIR)/*-wire-formats; do \
		name=$$(basename $$dir); \
		echo "  -> $$name"; \
		rsync -a --delete \
			--exclude='build/' \
			--exclude='CMakeLists.txt' \
			--exclude='README.md' \
			"$$dir/" "$(BLOG_POST_DIR)/$$name/"; \
	done
	@echo "Done. Synced wire-formats posts to $(BLOG_POST_DIR)"

# Include local overrides if present (gitignored)
-include Makefile.local
