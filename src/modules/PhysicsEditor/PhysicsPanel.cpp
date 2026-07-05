#include "PhysicsPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QSplitter>
#include <QScrollArea>

namespace Ks {

const QStringList PhysicsPanel::kSurfaceTypes = {
    "ROAD", "GRASS", "GRAVEL", "SAND", "KERB", "CONCRETE",
    "TARMAC", "METAL", "GLASS", "RUBBER", "DIRT", "SNOW", "ICE"
};

PhysicsPanel::PhysicsPanel(UndoStack* undoStack, QWidget* parent)
    : QWidget(parent), m_undo(undoStack)
{
    buildUI();
    setEnabled(false);
}

void PhysicsPanel::buildUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6,6,6,6);
    mainLayout->setSpacing(6);

    auto* nodeLabel = new QLabel("<b>No node selected</b>", this);
    nodeLabel->setObjectName("nodeLabel");
    mainLayout->addWidget(nodeLabel);

    auto* bodyGroup = new QGroupBox("Rigid Body", this);
    auto* bodyForm  = new QFormLayout(bodyGroup);

    m_bodyType = new QComboBox(this);
    m_bodyType->addItems({"Static", "Dynamic / Kinematic"});
    bodyForm->addRow("Type:", m_bodyType);

    m_mass = new QDoubleSpinBox(this);
    m_mass->setRange(0.0, 100000.0);
    m_mass->setSuffix(" kg");
    m_mass->setDecimals(1);
    bodyForm->addRow("Mass:", m_mass);

    m_linDamp = new QDoubleSpinBox(this);
    m_linDamp->setRange(0.0, 10.0);
    m_linDamp->setSingleStep(0.01);
    m_linDamp->setDecimals(3);
    bodyForm->addRow("Lin. Damping:", m_linDamp);

    m_angDamp = new QDoubleSpinBox(this);
    m_angDamp->setRange(0.0, 10.0);
    m_angDamp->setSingleStep(0.01);
    m_angDamp->setDecimals(3);
    bodyForm->addRow("Ang. Damping:", m_angDamp);

    mainLayout->addWidget(bodyGroup);

    auto* colListGroup  = new QGroupBox("Colliders", this);
    auto* colListLayout = new QVBoxLayout(colListGroup);

    m_colliderList = new QListWidget(this);
    m_colliderList->setMaximumHeight(120);
    colListLayout->addWidget(m_colliderList);

    auto* btnRow = new QHBoxLayout();
    m_addBtn    = new QPushButton("＋ Add", this);
    m_removeBtn = new QPushButton("－ Remove", this);
    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_removeBtn);
    colListLayout->addLayout(btnRow);
    mainLayout->addWidget(colListGroup);

    m_colliderGroup = new QGroupBox("Collider Properties", this);
    auto* colForm   = new QFormLayout(m_colliderGroup);

    m_colType = new QComboBox(this);
    m_colType->addItems({"Box", "Sphere", "Capsule", "Mesh (Static)", "Convex Hull"});
    colForm->addRow("Shape:", m_colType);

    auto* centerRow = new QHBoxLayout();
    m_cx = new QDoubleSpinBox(); m_cx->setRange(-1000,1000); m_cx->setSingleStep(0.1); m_cx->setDecimals(3);
    m_cy = new QDoubleSpinBox(); m_cy->setRange(-1000,1000); m_cy->setSingleStep(0.1); m_cy->setDecimals(3);
    m_cz = new QDoubleSpinBox(); m_cz->setRange(-1000,1000); m_cz->setSingleStep(0.1); m_cz->setDecimals(3);
    centerRow->addWidget(m_cx); centerRow->addWidget(m_cy); centerRow->addWidget(m_cz);
    colForm->addRow("Center XYZ:", centerRow);

    auto* extRow = new QHBoxLayout();
    m_hx = new QDoubleSpinBox(); m_hx->setRange(0.001,1000); m_hx->setSingleStep(0.1); m_hx->setValue(0.5); m_hx->setDecimals(3);
    m_hy = new QDoubleSpinBox(); m_hy->setRange(0.001,1000); m_hy->setSingleStep(0.1); m_hy->setValue(0.5); m_hy->setDecimals(3);
    m_hz = new QDoubleSpinBox(); m_hz->setRange(0.001,1000); m_hz->setSingleStep(0.1); m_hz->setValue(0.5); m_hz->setDecimals(3);
    extRow->addWidget(m_hx); extRow->addWidget(m_hy); extRow->addWidget(m_hz);
    colForm->addRow("Half-Extents:", extRow);

    m_radius = new QDoubleSpinBox(this);
    m_radius->setRange(0.001, 1000); m_radius->setSingleStep(0.1); m_radius->setValue(0.5); m_radius->setDecimals(3);
    colForm->addRow("Radius:", m_radius);

    m_height = new QDoubleSpinBox(this);
    m_height->setRange(0.001, 1000); m_height->setSingleStep(0.1); m_height->setValue(1.0); m_height->setDecimals(3);
    colForm->addRow("Height:", m_height);

    m_friction = new QDoubleSpinBox(this);
    m_friction->setRange(0.0, 10.0); m_friction->setSingleStep(0.05); m_friction->setValue(0.7); m_friction->setDecimals(2);
    colForm->addRow("Friction:", m_friction);

    m_restitution = new QDoubleSpinBox(this);
    m_restitution->setRange(0.0, 1.0); m_restitution->setSingleStep(0.05); m_restitution->setValue(0.3); m_restitution->setDecimals(2);
    colForm->addRow("Restitution:", m_restitution);

    m_isTrigger = new QCheckBox("Is Trigger (no collision response)", this);
    colForm->addRow("", m_isTrigger);

    m_surfaceType = new QComboBox(this);
    m_surfaceType->addItems(kSurfaceTypes);
    colForm->addRow("Surface Type:", m_surfaceType);

    mainLayout->addWidget(m_colliderGroup);
    mainLayout->addStretch();

    connect(m_addBtn,        &QPushButton::clicked,         this, &PhysicsPanel::onAddCollider);
    connect(m_removeBtn,     &QPushButton::clicked,         this, &PhysicsPanel::onRemoveCollider);
    connect(m_colliderList,  &QListWidget::currentRowChanged, this, &PhysicsPanel::onColliderSelected);
    connect(m_colType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PhysicsPanel::onColliderTypeChanged);
    connect(m_bodyType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PhysicsPanel::onBodyTypeChanged);

    auto changed = [this]{ onParamChanged(); };
    connect(m_mass,        &QDoubleSpinBox::editingFinished, changed);
    connect(m_linDamp,     &QDoubleSpinBox::editingFinished, changed);
    connect(m_angDamp,     &QDoubleSpinBox::editingFinished, changed);
    connect(m_cx,          &QDoubleSpinBox::editingFinished, changed);
    connect(m_cy,          &QDoubleSpinBox::editingFinished, changed);
    connect(m_cz,          &QDoubleSpinBox::editingFinished, changed);
    connect(m_hx,          &QDoubleSpinBox::editingFinished, changed);
    connect(m_hy,          &QDoubleSpinBox::editingFinished, changed);
    connect(m_hz,          &QDoubleSpinBox::editingFinished, changed);
    connect(m_radius,      &QDoubleSpinBox::editingFinished, changed);
    connect(m_height,      &QDoubleSpinBox::editingFinished, changed);
    connect(m_friction,    &QDoubleSpinBox::editingFinished, changed);
    connect(m_restitution, &QDoubleSpinBox::editingFinished, changed);
    connect(m_isTrigger,   &QCheckBox::toggled, changed);
    connect(m_surfaceType, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &PhysicsPanel::onSurfaceTypeChanged);
}

void PhysicsPanel::setScene(Scene* scene) {
    m_scene = scene;
    m_nodeName.clear();
    m_currentCollider = -1;
    setEnabled(false);
}

void PhysicsPanel::setSelectedNode(const QString& nodeName) {
    m_nodeName = nodeName;
    m_currentCollider = -1;
    setEnabled(m_scene != nullptr && !nodeName.isEmpty());

    auto* label = findChild<QLabel*>("nodeLabel");
    if (label) label->setText("<b>Node: </b>" + nodeName);

    refresh();
}

void PhysicsPanel::refresh() {
    if (!m_scene || m_nodeName.isEmpty()) return;

    PhysicsBody& body = m_scene->physicsBodies[m_nodeName];

    m_bodyType->setCurrentIndex(body.isStatic ? 0 : 1);
    m_mass->setValue(body.mass);
    m_linDamp->setValue(body.linearDamp);
    m_angDamp->setValue(body.angularDamp);

    populateColliderList();
}

void PhysicsPanel::populateColliderList() {
    m_colliderList->clear();
    if (!m_scene || m_nodeName.isEmpty()) return;
    const PhysicsBody& body = m_scene->physicsBodies.value(m_nodeName);
    for (int i = 0; i < body.colliders.size(); ++i) {
        const PhysicsCollider& col = body.colliders[i];
        static const QStringList typeNames = {"Box","Sphere","Capsule","Mesh","ConvexHull"};
        m_colliderList->addItem(QString("Collider %1 [%2]").arg(i).arg(typeNames.value((int)col.type)));
    }
    m_colliderGroup->setEnabled(false);
}

void PhysicsPanel::loadCollider(int idx) {
    if (!m_scene || m_nodeName.isEmpty()) return;
    const PhysicsBody& body = m_scene->physicsBodies.value(m_nodeName);
    if (idx < 0 || idx >= body.colliders.size()) return;
    const PhysicsCollider& col = body.colliders[idx];

    m_colType->setCurrentIndex((int)col.type);
    m_cx->setValue(col.center.x()); m_cy->setValue(col.center.y()); m_cz->setValue(col.center.z());
    m_hx->setValue(col.halfExtents.x()); m_hy->setValue(col.halfExtents.y()); m_hz->setValue(col.halfExtents.z());
    m_radius->setValue(col.radius);
    m_height->setValue(col.height);
    m_friction->setValue(col.friction);
    m_restitution->setValue(col.restitution);
    m_isTrigger->setChecked(col.isTrigger);
    m_surfaceType->setCurrentText(col.surfaceType.isEmpty() ? "ROAD" : col.surfaceType);
    m_colliderGroup->setEnabled(true);
    onColliderTypeChanged(m_colType->currentIndex());
}

void PhysicsPanel::saveCurrentCollider() {
    if (!m_scene || m_nodeName.isEmpty() || m_currentCollider < 0) return;
    PhysicsBody& body = m_scene->physicsBodies[m_nodeName];
    if (m_currentCollider >= body.colliders.size()) return;

    PhysicsCollider& col = body.colliders[m_currentCollider];
    col.type         = (ColliderType)m_colType->currentIndex();
    col.center       = {(float)m_cx->value(), (float)m_cy->value(), (float)m_cz->value()};
    col.halfExtents  = {(float)m_hx->value(), (float)m_hy->value(), (float)m_hz->value()};
    col.radius       = (float)m_radius->value();
    col.height       = (float)m_height->value();
    col.friction     = (float)m_friction->value();
    col.restitution  = (float)m_restitution->value();
    col.isTrigger    = m_isTrigger->isChecked();
    col.surfaceType  = m_surfaceType->currentText();
    m_scene->isDirty = true;
}

void PhysicsPanel::onAddCollider() {
    if (!m_scene || m_nodeName.isEmpty()) return;
    PhysicsCollider col;
    col.type = ColliderType::Box;
    m_scene->physicsBodies[m_nodeName].colliders.append(col);
    m_scene->isDirty = true;
    populateColliderList();
    m_colliderList->setCurrentRow(m_colliderList->count()-1);
    emit physicsChanged();
}

void PhysicsPanel::onRemoveCollider() {
    if (!m_scene || m_nodeName.isEmpty() || m_currentCollider < 0) return;
    m_scene->physicsBodies[m_nodeName].colliders.removeAt(m_currentCollider);
    m_currentCollider = -1;
    m_scene->isDirty = true;
    populateColliderList();
    emit physicsChanged();
}

void PhysicsPanel::onColliderSelected(int idx) {
    saveCurrentCollider();
    m_currentCollider = idx;
    loadCollider(idx);
}

void PhysicsPanel::onColliderTypeChanged(int idx) {
    bool isBox     = (idx == 0);
    bool isSphere  = (idx == 1);
    bool isCapsule = (idx == 2);
    m_hx->setVisible(isBox); m_hy->setVisible(isBox); m_hz->setVisible(isBox);
    m_radius->setVisible(isSphere || isCapsule);
    m_height->setVisible(isCapsule);
    onParamChanged();
}

void PhysicsPanel::onBodyTypeChanged(int idx) {
    if (!m_scene || m_nodeName.isEmpty()) return;
    bool isStatic = (idx == 0);
    m_scene->physicsBodies[m_nodeName].isStatic = isStatic;
    m_mass->setEnabled(!isStatic);
    m_scene->isDirty = true;
    emit physicsChanged();
}

void PhysicsPanel::onParamChanged() {
    saveCurrentCollider();
    if (m_scene) {
        m_scene->physicsBodies[m_nodeName].mass       = (float)m_mass->value();
        m_scene->physicsBodies[m_nodeName].linearDamp = (float)m_linDamp->value();
        m_scene->physicsBodies[m_nodeName].angularDamp= (float)m_angDamp->value();
        m_scene->isDirty = true;
    }
    emit physicsChanged();
}

void PhysicsPanel::onSurfaceTypeChanged(const QString&) { onParamChanged(); }

} // namespace Ks