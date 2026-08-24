import re
with open(r'E:\Users\Simon\source\repos\kseditor\ZERO_CHECK.vcxproj', 'r') as f:
    content = f.read()
# Remove all CustomBuild blocks
content = re.sub(r'<CustomBuild Include="E:.*?">.*?</CustomBuild>', '', content, flags=re.DOTALL)
with open(r'E:\Users\Simon\source\repos\kseditor\ZERO_CHECK.vcxproj', 'w') as f:
    f.write(content)
print('Done')