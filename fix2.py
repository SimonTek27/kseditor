lines = open('E:\\Users\\Simon\\source\\repos\\kseditor/src/core/mesh/MeshOperations.h').readlines()
for i in range(500, 512):
    if 'int segments = 8)' in lines[i]:
        lines[i] = lines[i].replace('int segments = 8)', 'int segments)')
with open('E:\\Users\\Simon\\source\\repos\\kseditor/src/core/mesh/MeshOperations.h', 'w') as f:
    f.writelines(lines)
print('Done')