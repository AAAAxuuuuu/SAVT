/****************************************************************************
** Meta object code from reading C++ file 'AnalysisController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../include/savt/ui/AnalysisController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AnalysisController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN4savt2ui18AnalysisControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto savt::ui::AnalysisController::qt_create_metaobjectdata<qt_meta_tag_ZN4savt2ui18AnalysisControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "savt::ui::AnalysisController",
        "projectRootPathChanged",
        "",
        "statusMessageChanged",
        "analysisReportChanged",
        "systemContextReportChanged",
        "astFileItemsChanged",
        "selectedAstFilePathChanged",
        "astPreviewTitleChanged",
        "astPreviewSummaryChanged",
        "astPreviewTextChanged",
        "capabilityNodeItemsChanged",
        "capabilityEdgeItemsChanged",
        "capabilityGroupItemsChanged",
        "capabilitySceneWidthChanged",
        "capabilitySceneHeightChanged",
        "analyzingChanged",
        "stopRequestedChanged",
        "analysisProgressChanged",
        "analysisPhaseChanged",
        "systemContextDataChanged",
        "systemContextCardsChanged",
        "aiAvailableChanged",
        "aiConfigPathChanged",
        "aiSetupMessageChanged",
        "aiBusyChanged",
        "aiHasResultChanged",
        "aiNodeNameChanged",
        "aiSummaryChanged",
        "aiResponsibilityChanged",
        "aiUncertaintyChanged",
        "aiCollaboratorsChanged",
        "aiEvidenceChanged",
        "aiNextActionsChanged",
        "aiStatusMessageChanged",
        "aiScopeChanged",
        "analyzeCurrentProject",
        "stopAnalysis",
        "analyzeProject",
        "projectRootPath",
        "analyzeProjectUrl",
        "projectRootUrl",
        "refreshAiAvailability",
        "clearAiExplanation",
        "requestAiExplanation",
        "QVariantMap",
        "nodeData",
        "userTask",
        "requestProjectAiExplanation",
        "copyCodeContextToClipboard",
        "nodeId",
        "copyTextToClipboard",
        "text",
        "statusMessage",
        "analysisReport",
        "systemContextReport",
        "astFileItems",
        "QVariantList",
        "selectedAstFilePath",
        "astPreviewTitle",
        "astPreviewSummary",
        "astPreviewText",
        "capabilityNodeItems",
        "capabilityEdgeItems",
        "capabilityGroupItems",
        "capabilitySceneWidth",
        "capabilitySceneHeight",
        "analyzing",
        "stopRequested",
        "analysisProgress",
        "analysisPhase",
        "systemContextData",
        "systemContextCards",
        "aiAvailable",
        "aiConfigPath",
        "aiSetupMessage",
        "aiBusy",
        "aiHasResult",
        "aiNodeName",
        "aiSummary",
        "aiResponsibility",
        "aiUncertainty",
        "aiCollaborators",
        "aiEvidence",
        "aiNextActions",
        "aiStatusMessage",
        "aiScope"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'projectRootPathChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'statusMessageChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'analysisReportChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'systemContextReportChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'astFileItemsChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'selectedAstFilePathChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'astPreviewTitleChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'astPreviewSummaryChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'astPreviewTextChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'capabilityNodeItemsChanged'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'capabilityEdgeItemsChanged'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'capabilityGroupItemsChanged'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'capabilitySceneWidthChanged'
        QtMocHelpers::SignalData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'capabilitySceneHeightChanged'
        QtMocHelpers::SignalData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'analyzingChanged'
        QtMocHelpers::SignalData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'stopRequestedChanged'
        QtMocHelpers::SignalData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'analysisProgressChanged'
        QtMocHelpers::SignalData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'analysisPhaseChanged'
        QtMocHelpers::SignalData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'systemContextDataChanged'
        QtMocHelpers::SignalData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'systemContextCardsChanged'
        QtMocHelpers::SignalData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiAvailableChanged'
        QtMocHelpers::SignalData<void()>(22, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiConfigPathChanged'
        QtMocHelpers::SignalData<void()>(23, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiSetupMessageChanged'
        QtMocHelpers::SignalData<void()>(24, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiBusyChanged'
        QtMocHelpers::SignalData<void()>(25, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiHasResultChanged'
        QtMocHelpers::SignalData<void()>(26, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiNodeNameChanged'
        QtMocHelpers::SignalData<void()>(27, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiSummaryChanged'
        QtMocHelpers::SignalData<void()>(28, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiResponsibilityChanged'
        QtMocHelpers::SignalData<void()>(29, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiUncertaintyChanged'
        QtMocHelpers::SignalData<void()>(30, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiCollaboratorsChanged'
        QtMocHelpers::SignalData<void()>(31, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiEvidenceChanged'
        QtMocHelpers::SignalData<void()>(32, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiNextActionsChanged'
        QtMocHelpers::SignalData<void()>(33, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiStatusMessageChanged'
        QtMocHelpers::SignalData<void()>(34, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'aiScopeChanged'
        QtMocHelpers::SignalData<void()>(35, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'analyzeCurrentProject'
        QtMocHelpers::MethodData<void()>(36, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'stopAnalysis'
        QtMocHelpers::MethodData<void()>(37, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'analyzeProject'
        QtMocHelpers::MethodData<void(const QString &)>(38, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 39 },
        }}),
        // Method 'analyzeProjectUrl'
        QtMocHelpers::MethodData<void(const QUrl &)>(40, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QUrl, 41 },
        }}),
        // Method 'refreshAiAvailability'
        QtMocHelpers::MethodData<void()>(42, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'clearAiExplanation'
        QtMocHelpers::MethodData<void()>(43, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'requestAiExplanation'
        QtMocHelpers::MethodData<void(const QVariantMap &, const QString &)>(44, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 45, 46 }, { QMetaType::QString, 47 },
        }}),
        // Method 'requestAiExplanation'
        QtMocHelpers::MethodData<void(const QVariantMap &)>(44, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { 0x80000000 | 45, 46 },
        }}),
        // Method 'requestProjectAiExplanation'
        QtMocHelpers::MethodData<void(const QString &)>(48, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 47 },
        }}),
        // Method 'requestProjectAiExplanation'
        QtMocHelpers::MethodData<void()>(48, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void),
        // Method 'copyCodeContextToClipboard'
        QtMocHelpers::MethodData<void(qulonglong)>(49, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::ULongLong, 50 },
        }}),
        // Method 'copyTextToClipboard'
        QtMocHelpers::MethodData<void(const QString &)>(51, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 52 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'projectRootPath'
        QtMocHelpers::PropertyData<QString>(39, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'statusMessage'
        QtMocHelpers::PropertyData<QString>(53, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'analysisReport'
        QtMocHelpers::PropertyData<QString>(54, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
        // property 'systemContextReport'
        QtMocHelpers::PropertyData<QString>(55, QMetaType::QString, QMC::DefaultPropertyFlags, 3),
        // property 'astFileItems'
        QtMocHelpers::PropertyData<QVariantList>(56, 0x80000000 | 57, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 4),
        // property 'selectedAstFilePath'
        QtMocHelpers::PropertyData<QString>(58, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
        // property 'astPreviewTitle'
        QtMocHelpers::PropertyData<QString>(59, QMetaType::QString, QMC::DefaultPropertyFlags, 6),
        // property 'astPreviewSummary'
        QtMocHelpers::PropertyData<QString>(60, QMetaType::QString, QMC::DefaultPropertyFlags, 7),
        // property 'astPreviewText'
        QtMocHelpers::PropertyData<QString>(61, QMetaType::QString, QMC::DefaultPropertyFlags, 8),
        // property 'capabilityNodeItems'
        QtMocHelpers::PropertyData<QVariantList>(62, 0x80000000 | 57, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 9),
        // property 'capabilityEdgeItems'
        QtMocHelpers::PropertyData<QVariantList>(63, 0x80000000 | 57, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 10),
        // property 'capabilityGroupItems'
        QtMocHelpers::PropertyData<QVariantList>(64, 0x80000000 | 57, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 11),
        // property 'capabilitySceneWidth'
        QtMocHelpers::PropertyData<double>(65, QMetaType::Double, QMC::DefaultPropertyFlags, 12),
        // property 'capabilitySceneHeight'
        QtMocHelpers::PropertyData<double>(66, QMetaType::Double, QMC::DefaultPropertyFlags, 13),
        // property 'analyzing'
        QtMocHelpers::PropertyData<bool>(67, QMetaType::Bool, QMC::DefaultPropertyFlags, 14),
        // property 'stopRequested'
        QtMocHelpers::PropertyData<bool>(68, QMetaType::Bool, QMC::DefaultPropertyFlags, 15),
        // property 'analysisProgress'
        QtMocHelpers::PropertyData<double>(69, QMetaType::Double, QMC::DefaultPropertyFlags, 16),
        // property 'analysisPhase'
        QtMocHelpers::PropertyData<QString>(70, QMetaType::QString, QMC::DefaultPropertyFlags, 17),
        // property 'systemContextData'
        QtMocHelpers::PropertyData<QVariantMap>(71, 0x80000000 | 45, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 18),
        // property 'systemContextCards'
        QtMocHelpers::PropertyData<QVariantList>(72, 0x80000000 | 57, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 19),
        // property 'aiAvailable'
        QtMocHelpers::PropertyData<bool>(73, QMetaType::Bool, QMC::DefaultPropertyFlags, 20),
        // property 'aiConfigPath'
        QtMocHelpers::PropertyData<QString>(74, QMetaType::QString, QMC::DefaultPropertyFlags, 21),
        // property 'aiSetupMessage'
        QtMocHelpers::PropertyData<QString>(75, QMetaType::QString, QMC::DefaultPropertyFlags, 22),
        // property 'aiBusy'
        QtMocHelpers::PropertyData<bool>(76, QMetaType::Bool, QMC::DefaultPropertyFlags, 23),
        // property 'aiHasResult'
        QtMocHelpers::PropertyData<bool>(77, QMetaType::Bool, QMC::DefaultPropertyFlags, 24),
        // property 'aiNodeName'
        QtMocHelpers::PropertyData<QString>(78, QMetaType::QString, QMC::DefaultPropertyFlags, 25),
        // property 'aiSummary'
        QtMocHelpers::PropertyData<QString>(79, QMetaType::QString, QMC::DefaultPropertyFlags, 26),
        // property 'aiResponsibility'
        QtMocHelpers::PropertyData<QString>(80, QMetaType::QString, QMC::DefaultPropertyFlags, 27),
        // property 'aiUncertainty'
        QtMocHelpers::PropertyData<QString>(81, QMetaType::QString, QMC::DefaultPropertyFlags, 28),
        // property 'aiCollaborators'
        QtMocHelpers::PropertyData<QVariantList>(82, 0x80000000 | 57, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 29),
        // property 'aiEvidence'
        QtMocHelpers::PropertyData<QVariantList>(83, 0x80000000 | 57, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 30),
        // property 'aiNextActions'
        QtMocHelpers::PropertyData<QVariantList>(84, 0x80000000 | 57, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 31),
        // property 'aiStatusMessage'
        QtMocHelpers::PropertyData<QString>(85, QMetaType::QString, QMC::DefaultPropertyFlags, 32),
        // property 'aiScope'
        QtMocHelpers::PropertyData<QString>(86, QMetaType::QString, QMC::DefaultPropertyFlags, 33),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AnalysisController, qt_meta_tag_ZN4savt2ui18AnalysisControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject savt::ui::AnalysisController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4savt2ui18AnalysisControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4savt2ui18AnalysisControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN4savt2ui18AnalysisControllerE_t>.metaTypes,
    nullptr
} };

void savt::ui::AnalysisController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AnalysisController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->projectRootPathChanged(); break;
        case 1: _t->statusMessageChanged(); break;
        case 2: _t->analysisReportChanged(); break;
        case 3: _t->systemContextReportChanged(); break;
        case 4: _t->astFileItemsChanged(); break;
        case 5: _t->selectedAstFilePathChanged(); break;
        case 6: _t->astPreviewTitleChanged(); break;
        case 7: _t->astPreviewSummaryChanged(); break;
        case 8: _t->astPreviewTextChanged(); break;
        case 9: _t->capabilityNodeItemsChanged(); break;
        case 10: _t->capabilityEdgeItemsChanged(); break;
        case 11: _t->capabilityGroupItemsChanged(); break;
        case 12: _t->capabilitySceneWidthChanged(); break;
        case 13: _t->capabilitySceneHeightChanged(); break;
        case 14: _t->analyzingChanged(); break;
        case 15: _t->stopRequestedChanged(); break;
        case 16: _t->analysisProgressChanged(); break;
        case 17: _t->analysisPhaseChanged(); break;
        case 18: _t->systemContextDataChanged(); break;
        case 19: _t->systemContextCardsChanged(); break;
        case 20: _t->aiAvailableChanged(); break;
        case 21: _t->aiConfigPathChanged(); break;
        case 22: _t->aiSetupMessageChanged(); break;
        case 23: _t->aiBusyChanged(); break;
        case 24: _t->aiHasResultChanged(); break;
        case 25: _t->aiNodeNameChanged(); break;
        case 26: _t->aiSummaryChanged(); break;
        case 27: _t->aiResponsibilityChanged(); break;
        case 28: _t->aiUncertaintyChanged(); break;
        case 29: _t->aiCollaboratorsChanged(); break;
        case 30: _t->aiEvidenceChanged(); break;
        case 31: _t->aiNextActionsChanged(); break;
        case 32: _t->aiStatusMessageChanged(); break;
        case 33: _t->aiScopeChanged(); break;
        case 34: _t->analyzeCurrentProject(); break;
        case 35: _t->stopAnalysis(); break;
        case 36: _t->analyzeProject((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 37: _t->analyzeProjectUrl((*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[1]))); break;
        case 38: _t->refreshAiAvailability(); break;
        case 39: _t->clearAiExplanation(); break;
        case 40: _t->requestAiExplanation((*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 41: _t->requestAiExplanation((*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[1]))); break;
        case 42: _t->requestProjectAiExplanation((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 43: _t->requestProjectAiExplanation(); break;
        case 44: _t->copyCodeContextToClipboard((*reinterpret_cast< std::add_pointer_t<qulonglong>>(_a[1]))); break;
        case 45: _t->copyTextToClipboard((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::projectRootPathChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::statusMessageChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::analysisReportChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::systemContextReportChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::astFileItemsChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::selectedAstFilePathChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::astPreviewTitleChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::astPreviewSummaryChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::astPreviewTextChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::capabilityNodeItemsChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::capabilityEdgeItemsChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::capabilityGroupItemsChanged, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::capabilitySceneWidthChanged, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::capabilitySceneHeightChanged, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::analyzingChanged, 14))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::stopRequestedChanged, 15))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::analysisProgressChanged, 16))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::analysisPhaseChanged, 17))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::systemContextDataChanged, 18))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::systemContextCardsChanged, 19))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiAvailableChanged, 20))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiConfigPathChanged, 21))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiSetupMessageChanged, 22))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiBusyChanged, 23))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiHasResultChanged, 24))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiNodeNameChanged, 25))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiSummaryChanged, 26))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiResponsibilityChanged, 27))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiUncertaintyChanged, 28))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiCollaboratorsChanged, 29))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiEvidenceChanged, 30))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiNextActionsChanged, 31))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiStatusMessageChanged, 32))
            return;
        if (QtMocHelpers::indexOfMethod<void (AnalysisController::*)()>(_a, &AnalysisController::aiScopeChanged, 33))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->projectRootPath(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->statusMessage(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->analysisReport(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->systemContextReport(); break;
        case 4: *reinterpret_cast<QVariantList*>(_v) = _t->astFileItems(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->selectedAstFilePath(); break;
        case 6: *reinterpret_cast<QString*>(_v) = _t->astPreviewTitle(); break;
        case 7: *reinterpret_cast<QString*>(_v) = _t->astPreviewSummary(); break;
        case 8: *reinterpret_cast<QString*>(_v) = _t->astPreviewText(); break;
        case 9: *reinterpret_cast<QVariantList*>(_v) = _t->capabilityNodeItems(); break;
        case 10: *reinterpret_cast<QVariantList*>(_v) = _t->capabilityEdgeItems(); break;
        case 11: *reinterpret_cast<QVariantList*>(_v) = _t->capabilityGroupItems(); break;
        case 12: *reinterpret_cast<double*>(_v) = _t->capabilitySceneWidth(); break;
        case 13: *reinterpret_cast<double*>(_v) = _t->capabilitySceneHeight(); break;
        case 14: *reinterpret_cast<bool*>(_v) = _t->analyzing(); break;
        case 15: *reinterpret_cast<bool*>(_v) = _t->stopRequested(); break;
        case 16: *reinterpret_cast<double*>(_v) = _t->analysisProgress(); break;
        case 17: *reinterpret_cast<QString*>(_v) = _t->analysisPhase(); break;
        case 18: *reinterpret_cast<QVariantMap*>(_v) = _t->systemContextData(); break;
        case 19: *reinterpret_cast<QVariantList*>(_v) = _t->systemContextCards(); break;
        case 20: *reinterpret_cast<bool*>(_v) = _t->aiAvailable(); break;
        case 21: *reinterpret_cast<QString*>(_v) = _t->aiConfigPath(); break;
        case 22: *reinterpret_cast<QString*>(_v) = _t->aiSetupMessage(); break;
        case 23: *reinterpret_cast<bool*>(_v) = _t->aiBusy(); break;
        case 24: *reinterpret_cast<bool*>(_v) = _t->aiHasResult(); break;
        case 25: *reinterpret_cast<QString*>(_v) = _t->aiNodeName(); break;
        case 26: *reinterpret_cast<QString*>(_v) = _t->aiSummary(); break;
        case 27: *reinterpret_cast<QString*>(_v) = _t->aiResponsibility(); break;
        case 28: *reinterpret_cast<QString*>(_v) = _t->aiUncertainty(); break;
        case 29: *reinterpret_cast<QVariantList*>(_v) = _t->aiCollaborators(); break;
        case 30: *reinterpret_cast<QVariantList*>(_v) = _t->aiEvidence(); break;
        case 31: *reinterpret_cast<QVariantList*>(_v) = _t->aiNextActions(); break;
        case 32: *reinterpret_cast<QString*>(_v) = _t->aiStatusMessage(); break;
        case 33: *reinterpret_cast<QString*>(_v) = _t->aiScope(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setProjectRootPath(*reinterpret_cast<QString*>(_v)); break;
        case 5: _t->setSelectedAstFilePath(*reinterpret_cast<QString*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *savt::ui::AnalysisController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *savt::ui::AnalysisController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4savt2ui18AnalysisControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int savt::ui::AnalysisController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 46)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 46;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 46)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 46;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 34;
    }
    return _id;
}

// SIGNAL 0
void savt::ui::AnalysisController::projectRootPathChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void savt::ui::AnalysisController::statusMessageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void savt::ui::AnalysisController::analysisReportChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void savt::ui::AnalysisController::systemContextReportChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void savt::ui::AnalysisController::astFileItemsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void savt::ui::AnalysisController::selectedAstFilePathChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void savt::ui::AnalysisController::astPreviewTitleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void savt::ui::AnalysisController::astPreviewSummaryChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void savt::ui::AnalysisController::astPreviewTextChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void savt::ui::AnalysisController::capabilityNodeItemsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void savt::ui::AnalysisController::capabilityEdgeItemsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void savt::ui::AnalysisController::capabilityGroupItemsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void savt::ui::AnalysisController::capabilitySceneWidthChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void savt::ui::AnalysisController::capabilitySceneHeightChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void savt::ui::AnalysisController::analyzingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void savt::ui::AnalysisController::stopRequestedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void savt::ui::AnalysisController::analysisProgressChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 16, nullptr);
}

// SIGNAL 17
void savt::ui::AnalysisController::analysisPhaseChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 17, nullptr);
}

// SIGNAL 18
void savt::ui::AnalysisController::systemContextDataChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 18, nullptr);
}

// SIGNAL 19
void savt::ui::AnalysisController::systemContextCardsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 19, nullptr);
}

// SIGNAL 20
void savt::ui::AnalysisController::aiAvailableChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 20, nullptr);
}

// SIGNAL 21
void savt::ui::AnalysisController::aiConfigPathChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 21, nullptr);
}

// SIGNAL 22
void savt::ui::AnalysisController::aiSetupMessageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 22, nullptr);
}

// SIGNAL 23
void savt::ui::AnalysisController::aiBusyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 23, nullptr);
}

// SIGNAL 24
void savt::ui::AnalysisController::aiHasResultChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 24, nullptr);
}

// SIGNAL 25
void savt::ui::AnalysisController::aiNodeNameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 25, nullptr);
}

// SIGNAL 26
void savt::ui::AnalysisController::aiSummaryChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 26, nullptr);
}

// SIGNAL 27
void savt::ui::AnalysisController::aiResponsibilityChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 27, nullptr);
}

// SIGNAL 28
void savt::ui::AnalysisController::aiUncertaintyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 28, nullptr);
}

// SIGNAL 29
void savt::ui::AnalysisController::aiCollaboratorsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 29, nullptr);
}

// SIGNAL 30
void savt::ui::AnalysisController::aiEvidenceChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 30, nullptr);
}

// SIGNAL 31
void savt::ui::AnalysisController::aiNextActionsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 31, nullptr);
}

// SIGNAL 32
void savt::ui::AnalysisController::aiStatusMessageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 32, nullptr);
}

// SIGNAL 33
void savt::ui::AnalysisController::aiScopeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 33, nullptr);
}
QT_WARNING_POP
