// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "CppDocument.h"
#include "LookupItem.h"
#include "AlreadyConsideredClassContainer.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/Type.h>
#include <cplusplus/SymbolVisitor.h>
#include <cplusplus/Control.h>
#include <cplusplus/Name.h>

#include <QSet>
#include <QMap>

#include <functional>
#include <map>
#include <unordered_map>

namespace CPlusPlus {

namespace Internal {
struct FullyQualifiedName
{
    QList<const Name *> fqn;

    FullyQualifiedName(const QList<const Name *> &fqn)
        : fqn(fqn)
    {}
};
} // namespace Internal;

class CreateBindings;

class CPLUSPLUS_EXPORT ClassOrNamespace
{
    Q_DISABLE_COPY(ClassOrNamespace)

    ClassOrNamespace(CreateBindings *factory, ClassOrNamespace *parent);

public:
    ~ClassOrNamespace();

    const TemplateNameId *templateId() const;
    ClassOrNamespace *instantiationOrigin() const;

    ClassOrNamespace *parent() const;
    const QList<ClassOrNamespace *> usings() const;
    QList<Enum *> unscopedEnums() const;
    const QList<Symbol *> symbols() const;

    ClassOrNamespace *globalNamespace() const;

    QList<LookupItem> lookup(const Name *name);
    QList<LookupItem> find(const Name *name);

    ClassOrNamespace *lookupType(const Name *name);
    ClassOrNamespace *lookupType(const Name *name, Block *block);
    ClassOrNamespace *findType(const Name *name);
    ClassOrNamespace *findBlock(Block *block);
    ClassOrNamespace *getNested(const Name *name);

    Symbol *lookupInScope(const QList<const Name *> &fullName);

    /// The class this ClassOrNamespace is based on.
    Class *rootClass() const { return _rootClass; }

private:
    typedef std::unordered_map<const Name *, ClassOrNamespace *, Name::Hash, Name::Equals> Table;
    typedef std::unordered_map<const TemplateNameId *, ClassOrNamespace *, TemplateNameId::Hash, TemplateNameId::Equals> TemplateNameIdTable;
    typedef QHash<const AnonymousNameId *, ClassOrNamespace *> Anonymouses;

    /// \internal
    void flush();

    /// \internal
    ClassOrNamespace *findOrCreateType(const Name *name, ClassOrNamespace *origin = nullptr,
                                       Class *clazz = nullptr);

    ClassOrNamespace *findOrCreateNestedAnonymousType(const AnonymousNameId *anonymousNameId);

    void addTodo(Symbol *symbol);
    void addSymbol(Symbol *symbol);
    void addUnscopedEnum(Enum *e);
    void addUsing(ClassOrNamespace *u);
    void addNestedType(const Name *alias, ClassOrNamespace *e);

    QList<LookupItem> lookup_helper(const Name *name, bool searchInEnclosingScope);

    void lookup_helper(const Name *name, ClassOrNamespace *binding,
                       QList<LookupItem> *result,
                       QSet<ClassOrNamespace *> *processed,
                       const TemplateNameId *templateId);

    ClassOrNamespace *lookupType_helper(const Name *name, QSet<ClassOrNamespace *> *processed,
                                        bool searchInEnclosingScope, ClassOrNamespace *origin);

    ClassOrNamespace *findBlock_helper(Block *block, QSet<ClassOrNamespace *> *processed,
                                       bool searchInEnclosingScope);

    ClassOrNamespace *nestedType(const Name *name, QSet<ClassOrNamespace *> *processed,
                                 ClassOrNamespace *origin);

    void instantiateNestedClasses(ClassOrNamespace *enclosingTemplateClass,
                                  Clone &cloner,
                                  Subst &subst,
                                  ClassOrNamespace *enclosingTemplateClassInstantiation);
    ClassOrNamespace *findSpecialization(const TemplateNameId *templId,
                                         const TemplateNameIdTable &specializations);

    CreateBindings *_factory;
    ClassOrNamespace *_parent;
    QList<Symbol *> _symbols;
    QList<ClassOrNamespace *> _usings;
    Table _classOrNamespaces;
    QHash<Block *, ClassOrNamespace *> _blocks;
    QList<Enum *> _enums;
    QList<Symbol *> _todo;
    QSharedPointer<Control> _control;
    TemplateNameIdTable _specializations;
    QMap<const TemplateNameId *, ClassOrNamespace *> _instantiations;
    Anonymouses _anonymouses;
    QSet<const AnonymousNameId *> _declaredOrTypedefedAnonymouses;

    QHash<Internal::FullyQualifiedName, Symbol *> *_scopeLookupCache;

    // it's an instantiation.
    const TemplateNameId *_templateId;
    ClassOrNamespace *_instantiationOrigin;

    AlreadyConsideredClassContainer<Class> _alreadyConsideredClasses;
    AlreadyConsideredClassContainer<TemplateNameId> _alreadyConsideredTemplates;

    Class *_rootClass;

    class NestedClassInstantiator
    {
    public:
        NestedClassInstantiator(CreateBindings *factory, Clone &cloner, Subst &subst)
            : _factory(factory)
            , _cloner(cloner)
            , _subst(subst)
        {}
        void instantiate(ClassOrNamespace *enclosingTemplateClass,
                         ClassOrNamespace *enclosingTemplateClassInstantiation);
    private:
        bool isInstantiateNestedClassNeeded(const QList<Symbol *> &symbols) const;
        bool containsTemplateType(Declaration *declaration) const;
        bool containsTemplateType(Function *function) const;
        NamedType *findNamedType(Type *memberType) const;

        QSet<ClassOrNamespace *> _alreadyConsideredNestedClassInstantiations;
        CreateBindings *_factory;
        Clone &_cloner;
        Subst &_subst;
    };

public:
    const Name *_name; // For debug

    friend class CreateBindings;
};

class CPLUSPLUS_EXPORT CreateBindings: protected SymbolVisitor
{
    Q_DISABLE_COPY(CreateBindings)

public:
    CreateBindings(Document::Ptr thisDocument, const Snapshot &snapshot);
    virtual ~CreateBindings();

    /// Returns the binding for the global namespace.
    ClassOrNamespace *globalNamespace() const;

    /// Finds the binding associated to the given symbol.
    ClassOrNamespace *lookupType(Symbol *symbol, ClassOrNamespace *enclosingBinding = nullptr);
    ClassOrNamespace *lookupType(const QList<const Name *> &path,
                                 ClassOrNamespace *enclosingBinding = nullptr);

    /// Returns the Control that must be used to create temporary symbols.
    /// \internal
    QSharedPointer<Control> control() const
    { return _control; }

    bool expandTemplates() const
    { return _expandTemplates; }
    void setExpandTemplates(bool expandTemplates)
    { _expandTemplates = expandTemplates; }

    /// Searches in \a scope for symbols with the given \a name.
    /// Store the result in \a results.
    /// \internal
    void lookupInScope(const Name *name, Scope *scope, QList<LookupItem> *result,
                            const TemplateNameId *templateId, ClassOrNamespace *binding);

    /// Create bindings for the symbols reachable from \a rootSymbol.
    /// \internal
    void process(Symbol *rootSymbol, ClassOrNamespace *classOrNamespace);

    /// Create an empty ClassOrNamespace binding with the given \a parent.
    /// \internal
    ClassOrNamespace *allocClassOrNamespace(ClassOrNamespace *parent);

protected:
    using SymbolVisitor::visit;

    /// Change the current ClassOrNamespace binding.
    ClassOrNamespace *switchCurrentClassOrNamespace(ClassOrNamespace *classOrNamespace);

    /// Enters the ClassOrNamespace binding associated with the given \a symbol.
    ClassOrNamespace *enterClassOrNamespaceBinding(Symbol *symbol);

    /// Enters a ClassOrNamespace binding for the given \a symbol in the global
    /// namespace binding.
    ClassOrNamespace *enterGlobalClassOrNamespace(Symbol *symbol);

    /// Creates bindings for the given \a document.
    void process(Document::Ptr document);

    /// Creates bindings for the symbols reachable from the \a root symbol.
    void process(Symbol *root);

    bool visit(Template *templ) override;
    bool visit(Namespace *ns) override;
    bool visit(Class *klass) override;
    bool visit(ForwardClassDeclaration *klass) override;
    bool visit(Enum *e) override;
    bool visit(Declaration *decl) override;
    bool visit(Function *function) override;
    bool visit(Block *block) override;

    bool visit(BaseClass *b) override;
    bool visit(UsingNamespaceDirective *u) override;
    bool visit(UsingDeclaration *u) override;
    bool visit(NamespaceAlias *a) override;

    bool visit(ObjCClass *klass) override;
    bool visit(ObjCBaseClass *b) override;
    bool visit(ObjCForwardClassDeclaration *klass) override;
    bool visit(ObjCProtocol *proto) override;
    bool visit(ObjCBaseProtocol *b) override;
    bool visit(ObjCForwardProtocolDeclaration *proto) override;
    bool visit(ObjCMethod *) override;

private:
    Symbol *instantiateTemplateFunction(const Name *instantiationName,
                                        Template *specialization) const;

    Snapshot _snapshot;
    QSharedPointer<Control> _control;
    QSet<Namespace *> _processed;
    QList<ClassOrNamespace *> _entities;
    ClassOrNamespace *_globalNamespace;
    ClassOrNamespace *_currentClassOrNamespace;
    bool _expandTemplates;
};

class CPLUSPLUS_EXPORT LookupContext
{
public:
    LookupContext();

    LookupContext(Document::Ptr thisDocument,
                  const Snapshot &snapshot);

    LookupContext(Document::Ptr expressionDocument,
                  Document::Ptr thisDocument,
                  const Snapshot &snapshot,
                  QSharedPointer<CreateBindings> bindings = QSharedPointer<CreateBindings>());

    LookupContext(const LookupContext &other);
    LookupContext &operator = (const LookupContext &other);

    Document::Ptr expressionDocument() const;
    Document::Ptr thisDocument() const;
    Document::Ptr document(const QString &fileName) const;
    Snapshot snapshot() const;

    ClassOrNamespace *globalNamespace() const;

    QList<LookupItem> lookup(const Name *name, Scope *scope) const;
    ClassOrNamespace *lookupType(const Name *name, Scope *scope,
                                 ClassOrNamespace *enclosingBinding = nullptr,
                                 QSet<const Declaration *> typedefsBeingResolved
                                    = QSet<const Declaration *>()) const;
    ClassOrNamespace *lookupType(Symbol *symbol,
                                 ClassOrNamespace *enclosingBinding = nullptr) const;
    ClassOrNamespace *lookupParent(Symbol *symbol) const;

    /// \internal
    QSharedPointer<CreateBindings> bindings() const
    { return _bindings; }

    enum InlineNamespacePolicy { ShowInlineNamespaces, HideInlineNamespaces };
    static QList<const Name *> fullyQualifiedName(
        Symbol *symbol, InlineNamespacePolicy policy = ShowInlineNamespaces);
    static QList<const Name *> path(Symbol *symbol,
                                    InlineNamespacePolicy policy = ShowInlineNamespaces);

    static const Name *minimalName(Symbol *symbol, ClassOrNamespace *target, Control *control);

    void setExpandTemplates(bool expandTemplates)
    {
        if (_bindings)
            _bindings->setExpandTemplates(expandTemplates);
        m_expandTemplates = expandTemplates;
    }

private:
    QList<LookupItem> lookupByUsing(const Name *name, ClassOrNamespace *bindingScope) const;

    // The current expression.
    Document::Ptr _expressionDocument;

    // The current document.
    Document::Ptr _thisDocument;

    // All documents.
    Snapshot _snapshot;

    // Bindings
    QSharedPointer<CreateBindings> _bindings;

    bool m_expandTemplates;
};

bool CPLUSPLUS_EXPORT compareFullyQualifiedName(const QList<const Name *> &path,
                                                const QList<const Name *> &other);

} // CPlusPlus
