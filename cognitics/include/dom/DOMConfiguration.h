/****************************************************************************
Copyright (c) 2019 Cognitics, Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
****************************************************************************/
/*! \file dom/DOMConfiguration.h
\headerfile dom/DOMConfiguration.h
\brief Provides dom::DOMConfiguration.
\author Aaron Brinton <abrinton@cognitics.net>
\date 02 October 2009
\sa Document Object Model (DOM) Level 3 Core Specification (http://www.w3.org/TR/DOM-Level-3-Core/) 1.4
*/
#pragma once

#include "dom/base.h"

namespace dom
{
/*! \class dom::DOMConfiguration DOMConfiguration.h DOMConfiguration.h
\brief Configuration

The DOMConfiguration interface represents the configuration of a document and maintains a table of recognized parameters.
Using the configuration, it is possible to change Document::normalizeDocument() behavior, such as replacing the CDATASection nodes with Text nodes or specifying the type of the schema that must be used when the validation of the Document is requested.
DOMConfiguration objects are also used in DOM Level 3 Load and Save in the DOMParser and DOMSerializer interfaces.

The parameter names used by the DOMConfiguration object are defined throughout the DOM Level 3 specifications.
Names are case-insensitive.
To avoid possible conflicts, as a convention, names referring to parameters defined outside the DOM specification should be made unique.
Because parameters are exposed as properties in the ECMAScript Language Binding, names are recommended to follow the section "5.16 Identifiers" of Unicode with the addition of the character '-' (HYPHEN-MINUS) but it is not enforced by the DOM implementation.
DOM Level 3 Core Implementations are required to recognize all parameters defined in this specification.
Some parameter values may also be required to be supported by the implementation.
Refer to the definition of the parameter to know if a value must be supported or not.

\note Parameters are similar to features and properties used in SAX2.

The following list of parameters are defined in the DOM:
\li "canonical-form"
- true [optional]
    - Canonicalize the document according to the rules specified in Canonical XML, such as removing the DocumentType node (if any) from the tree, or removing superfluous namespace declarations from each element.
    Note that this is limited to what can be represented in the DOM; in particular, there is no way to specify the order of the attributes in the DOM.
    In addition, setting this parameter to true will also set the state of the parameters listed below.
    Later changes to the state of one of those parameters will revert "canonical-form" back to false.
    - Parameters set to false: "entities", "normalize-characters", "cdata-sections".
    - Parameters set to true: "namespaces", "namespace-declarations", "well-formed", "element-content-whitespace".
    -Other parameters are not changed unless explicitly specified in the description of the parameters.
- false [required] (default)
    - Do not canonicalize the document.
\li "cdata-sections"
- true [required] (default)
    - Keep CDATASection nodes in the document.
- false [required]
    - Transform CDATASection nodes in the document into Text nodes.
    The new Text node is then combined with any adjacent Text node.
\li "check-character-normalization"
- true [optional]
    - Check if the characters in the document are fully normalized, as defined in appendix B of XML 1.1.
    When a sequence of characters is encountered that fails normalization checking, an error with the DOMError::type equals to "check-character-normalization-failure" is issued.
- false [required] (default)
    - Do not check if characters are normalized.
\li "comments"
- true [required] (default)
    - Keep Comment nodes in the document.
- false [required]
    - Discard Comment nodes in the document.
\li "datatype-normalization"
- true [optional]
    - Expose schema normalized values in the tree, such as XML Schema normalized values in the case of XML Schema.
    Since this parameter requires to have schema information, the "validate" parameter will also be set to true.
    Having this parameter activated when "validate" is false has no effect and no schema-normalization will happen.
    \note Since the document contains the result of the XML 1.0 processing, this parameter does not apply to attribute value normalization as defined in section 3.3.3 of XML 1.0 and is only meant for schema languages other than Document Type Definition (DTD).
- false [required] (default)
    - Do not perform schema normalization on the tree.
\li "element-content-whitespace"
- true [required] (default)
    - Keep all whitespaces in the document.
- false [optional]
    - Discard all Text nodes that contain whitespaces in element content.
    The implementation is expected to use the attribute Text::isElementContentWhitespace to determine if a Text node should be discarded or not.
\li "entities"
- true [required] (default)
    - Keep EntityReference nodes in the document.
- false [required]
    - Remove all EntityReference nodes from the document, putting the entity expansions directly in their place.
    Text nodes are normalized, as defined in Node::normalize.
    Only unexpanded entity references are kept in the document.
- Note: This parameter does not affect Entity nodes.
\li "error-handler" [required]
- Contains a DOMErrorHandler object.
If an error is encountered in the document, the implementation will call back the DOMErrorHandler registered using this parameter.
The implementation may provide a default DOMErrorHandler object.
- When called, DOMError::relatedData will contain the closest node to where the error occurred.
If the implementation is unable to determine the node where the error occurs, DOMError::relatedData will contain the Document node.
Mutations to the document from within an error handler will result in implementation dependent behavior.
\li "infoset"
- true [required]
    - Keep in the document the information defined in the XML Information Set.
    - This forces the following parameters to false: "validate-if-schema", "entities", "datatype-normalization", "cdata-sections".
    - This forces the following parameters to true: "namespace-declarations", "well-formed", "element-content-whitespace", "comments", "namespaces".
    - Other parameters are not changed unless explicitly specified in the description of the parameters.
    - Note that querying this parameter with getParameter returns true only if the individual parameters specified above are appropriately set.
- false
    - Setting infoset to false has no effect.
\li "namespaces"
- true [required] (default)
    - Perform the namespace processing as defined in Namespace Normalization.
- false [optional]
    - Do not perform the namespace processing.
\li "namespace-declarations"
- This parameter has no effect if the parameter "namespaces" is set to false.
- true [required] (default)
    - Include namespace declaration attributes, specified or defaulted from the schema, in the document. See also the sections "Declaring Namespaces" in XML Namespaces and XML Namespaces 1.1.
- false [required]
    - Discard all namespace declaration attributes.
    The namespace prefixes (Node.prefix) are retained even if this parameter is set to false.
\li "normalize-characters"
- true [optional]
    - Fully normalized the characters in the document as defined in appendix B of XML 1.1.
- false [required] (default)
    - Do not perform character normalization.
\li "schema-location" [optional]
- Represent a DOMString object containing a list of URIs, separated by whitespaces (characters matching the nonterminal production S defined in section 2.3 XML 1.0), that represents the schemas against which validation should occur, i.e. the current schema.
The types of schemas referenced in this list must match the type specified with schema-type, otherwise the behavior of an implementation is undefined.
- The schemas specified using this property take precedence to the schema information specified in the document itself.
For namespace aware schema, if a schema specified using this property and a schema specified in the document instance (i.e. using the schemaLocation attribute) in a schema document (i.e. using schema import mechanisms) share the same targetNamespace, the schema specified by the user using this property will be used.
If two schemas specified using this property share the same targetNamespace or have no namespace, the behavior is implementation dependent.
- If no location has been provided, this parameter is null.
- Note: The "schema-location" parameter is ignored unless the "schema-type" parameter value is set.
It is strongly recommended that Document.documentURI will be set so that an implementation can successfully resolve any external entities referenced.
\li "schema-type" [optional]
- Represent a DOMString object containing an absolute URI and representing the type of the schema language used to validate a document against.
Note that no lexical checking is done on the absolute URI.
- If this parameter is not set, a default value may be provided by the implementation, based on the schema languages supported and on the schema language used at load time.
If no value is provided, this parameter is null.
- Note: For XML Schema, applications must use the value "http://www.w3.org/2001/XMLSchema".
For XML DTD [XML 1.0], applications must use the value "http://www.w3.org/TR/REC-xml".
Other schema languages are outside the scope of the W3C and therefore should recommend an absolute URI in order to use this method.
\li "split-cdata-sections"
- true [required] (default)
    - Split CDATA sections containing the CDATA section termination marker ']]>'.
    When a CDATA section is split a warning is issued with a DOMError::type equals to "cdata-sections-splitted" and DOMError::relatedData equals to the first CDATASection node in document order resulting from the split.
- false [required]
    - Signal an error if a CDATASection contains an unrepresentable character.
\li "validate"
- true [optional]
    - Require the validation against a schema (i.e. XML schema, DTD, any other type or representation of schema) of the document as it is being normalized as defined by XML 1.0.
    If validation errors are found, or no schema was found, the error handler is notified.
    - Schema-normalized values will not be exposed according to the schema in used unless the parameter "datatype-normalization" is true.
    - This parameter will reevaluate:
        - Attribute nodes with Attr::specified equals to false, as specified in the description of the Attr interface;
        - The value of the attribute Text::isElementContentWhitespace for all Text nodes;
        - The value of the attribute Attr::isId for all Attr nodes;
        - The attributes Element::schemaTypeInfo and Attr::schemaTypeInfo.
    - Note: "validate-if-schema" and "validate" are mutually exclusive, setting one of them to true will set the other one to false.
    Applications should also consider setting the parameter "well-formed" to true, which is the default for that option, when validating the document.
- false [required] (default)
    - Do not accomplish schema processing, including the internal subset processing.
    Default attribute values information are kept. Note that validation might still happen if "validate-if-schema" is true.
\li "validate-if-schema"
- true [optional]
    - Enable validation only if a declaration for the document element can be found in a schema (independently of where it is found, i.e. XML schema, DTD, or any other type or representation of schema).
    If validation is enabled, this parameter has the same behavior as the parameter "validate" set to true.
    - Note: "validate-if-schema" and "validate" are mutually exclusive, setting one of them to true will set the other one to false.
- false [required] (default)
    - No schema processing should be performed if the document has a schema, including internal subset processing.
    Default attribute values information are kept.
    Note that validation must still happen if "validate" is true.
\li "well-formed"
- true [required] (default)
    - Check if all nodes are XML well formed according to the XML version in use in Document::xmlVersion:
        - check if the attribute Node::nodeName contains invalid characters according to its node type and generate a DOMError of type "wf-invalid-character-in-node-name", with a DOMError::SEVERITY_ERROR severity, if necessary;
        - check if the text content inside Attr, Element, Comment, Text, CDATASection nodes for invalid characters and generate a DOMError of type "wf-invalid-character", with a DOMError::SEVERITY_ERROR severity, if necessary;
        - check if the data inside ProcessingInstruction nodes for invalid characters and generate a DOMError of type "wf-invalid-character", with a DOMError::SEVERITY_ERROR severity, if necessary;
- false [optional]
    - Do not check for XML well-formedness.

The resolution of the system identifiers associated with entities is done using Document::documentURI.
However, when the feature "LS" defined in DOM Level 3 Load and Save is supported by the DOM implementation, the parameter "resource-resolver" can also be used on DOMConfiguration objects attached to Document nodes.
If this parameter is set, Document::normalizeDocument() will invoke the resource resolver instead of using Document::documentURI.

\sa Document Object Model (DOM) Level 3 Core Specification (http://www.w3.org/TR/DOM-Level-3-Core/) 1.4 [p106]
\note Introduced in DOM Level 3.
*/
    class DOMConfiguration : public DOMObject
    {
    private:
        struct _DOMConfiguration;
        _DOMConfiguration *_data;

    public:
        virtual ~DOMConfiguration(void);
        DOMConfiguration(void);

/*! \brief Get the list of parameters supported by this DOMConfiguration object.
\return The list of parameters supported by this DOMConfiguration object.
*/
        virtual DOMStringList getParameterNames(void);

/*! \brief Set the value of a parameter.
\param name The name of the parameter to set.
\param value The new value or null if the user wishes to unset the parameter.
While the type of the value parameter is defined as DOMUserData, the object type must match the type defined by the definition of the parameter.
For example, if the parameter is "error-handler", the value must be of type DOMErrorHandler.
\exception NOT_FOUND_ERR the parameter name is not recognized.
\exception NOT_SUPPORTED_ERR the parameter name is recognized but the requested value cannot be set.
\exception TYPE_MISMATCH_ERR the value type for this parameter name is incompatible with the expected value type.
*/
        virtual void setParameter(const DOMString &name, DOMUserDataSP value);

/*! \brief Get the value of a parameter if known.
\param name The name of the parameter.
\return The current object associated with the specified parameter or null if no object has been associated or if the parameter is not supported.
\exception NOT_FOUND_ERR the parameter name is not recognized.
*/
        virtual DOMUserDataSP getParameter(const DOMString &name);

/*! \brief Check if setting a parameter to a specific value is supported.
\param name The name of the parameter to check.
\param value An object. If empty, the returned value is true.
\return True if the parameter could be successfully set to the specified value, or false if the parameter is not recognized or the requested value is not supported.
This does not change the current value of the parameter itself.
*/
        // TODO: virtual bool canSetParameter(const DOMString &name, DOMUserDataSP value);

    };


}
