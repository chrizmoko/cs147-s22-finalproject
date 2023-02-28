"""JSON dict error."""


from enum import Enum
from typing import Dict, Union


class ErrorBuilderError(Exception):
    pass


class ErrorType(Enum):
    INVALID_REQUEST = 'invalid_request'
    UNAUTHORIZED_REQUEST = 'unauthorized_request'
    SERVER_ERROR = 'server_error'

    def __str__(self) -> str:
        return self.value


class ErrorBuilder:
    def __init__(self, error_type: ErrorType, error_description: str = '') -> None:
        self._type = error_type
        self._description = error_description
    
    def build(self) -> Dict[str, Union[str, Dict]]:
        return {
            'error': str(self._type),
            'error_description': self._description,
        }


class RequestErrorBuilder(ErrorBuilder):
    def __init__(self, error_description: str = None) -> None:
        if error_description == None:
            error_description = 'Request was invalid and was not processed'
        super().__init__(ErrorType.INVALID_REQUEST, error_description)
        self._argument_reasons: Dict[str, str] = {}
    
    def add_argument_required(self, argument: str) -> None:
        self.add_custom(argument, f'{argument} is required')
    
    def add_invalid_argument_format(self, argument: str) -> None:
        self.add_custom(argument, f'{argument} is not formatted properly')
    
    def add_invalid_argument_value(self, argument: str) -> None:
        self.add_custom(argument, f'{argument} is not a valid value')

    def add_invalid_argument_type(self, argument: str) -> None:
        self.add_custom(argument, f'{argument} is not the expected type')

    def add_custom(self, argument: str, reason: str) -> None:
        self._check_for_repeated_argument(argument)
        self._argument_reasons[argument] = reason

    def build(self) -> Dict[str, Union[str, Dict]]:
        base = super().build()
        base['error_details'] = self._argument_reasons
        return base
    
    def _check_for_repeated_argument(self, argument: str) -> None:
        if argument in self._argument_reasons:
            raise ErrorBuilderError('Argument duplicated')