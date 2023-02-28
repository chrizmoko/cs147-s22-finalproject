.PHONY: run debug dependencies clean


PYTHON_ALIAS := python3
PIP_ALIAS := $(PYTHON_ALIAS) -m pip
FLASK_ALIAS := $(PYTHON_ALIAS) -m flask

APPLICATION_MODULE := helloworld
APPLICATION_HANDLE := $(APPLICATION_MODULE)/app


# Runs the flask development server (to everyone)
run:
	export FLASK_APP=$(APPLICATION_HANDLE) && \
	$(FLASK_ALIAS) run --host=0.0.0.0

# Runs the flask development server with live debug and editing
debug:
	export FLASK_APP=$(APPLICATION_HANDLE) && \
	export FLASK_ENV=development && \
	$(FLASK_ALIAS) run

# Download and install project dependencies from requirements.txt
dependencies:
	$(PIP_ALIAS) install -r requirements.txt

# Removes all the python-generated files that are not source
clean:
	find $(APPLICATION_MODULE) -name *.pyc -exec rm -f {} +
	find $(APPLICATION_MODULE) -name __pycache__ -exec rm -rf {} +

