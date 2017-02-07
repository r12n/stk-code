layout(location = 0) in vec3 Position;

subroutine mat4 projViewMatrixType();
subroutine uniform projViewMatrixType getPVM;

subroutine(projViewMatrixType) mat4 normal()
{
    return ProjectionViewMatrix;
}

subroutine(projViewMatrixType) mat4 shadow1()
{
    return ShadowViewProjMatrixes[0];
}

subroutine(projViewMatrixType) mat4 shadow2()
{
    return ShadowViewProjMatrixes[1];
}

subroutine(projViewMatrixType) mat4 shadow3()
{
    return ShadowViewProjMatrixes[2];
}

subroutine(projViewMatrixType) mat4 shadow4()
{
    return ShadowViewProjMatrixes[3];
}

void main(void)
{
    gl_Position = getPVM() * vec4(Position, 1.);
}
