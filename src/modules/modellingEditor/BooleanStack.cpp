#include "BooleanStack.h"

namespace ks {

BooleanStack::BooleanStack(QObject* parent)
    : QObject(parent)
{
}

bool BooleanStack::add(int operation, int operandId, const QString& operandName)
{
    if (operandId < 0) return false;
    BooleanOp op;
    op.operation = qBound(0, operation, 3);
    op.operandId = operandId;
    op.operandName = operandName;
    op.enabled = true;
    m_ops.append(op);
    emit changed();
    return true;
}

bool BooleanStack::remove(int index)
{
    if (index < 0 || index >= m_ops.size()) return false;
    m_ops.removeAt(index);
    emit changed();
    return true;
}

bool BooleanStack::move(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= m_ops.size()) return false;
    if (toIndex < 0 || toIndex >= m_ops.size()) return false;
    m_ops.move(fromIndex, toIndex);
    emit changed();
    return true;
}

bool BooleanStack::setEnabled(int index, bool enabled)
{
    if (index < 0 || index >= m_ops.size()) return false;
    m_ops[index].enabled = enabled;
    emit changed();
    return true;
}

bool BooleanStack::setOperation(int index, int operation)
{
    if (index < 0 || index >= m_ops.size()) return false;
    m_ops[index].operation = qBound(0, operation, 3);
    emit changed();
    return true;
}

void BooleanStack::clear()
{
    if (m_ops.isEmpty()) return;
    m_ops.clear();
    emit changed();
}

} // namespace ks
