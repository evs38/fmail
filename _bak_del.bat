@ECHO OFF
FOR /R %%B IN (*.BAK)  DO DEL /F %%B
FOR /R %%B IN (*.~C*)  DO DEL /F %%B
FOR /R %%B IN (*.~H*)  DO DEL /F %%B
FOR /R %%B IN (*.~???) DO DEL /F %%B
