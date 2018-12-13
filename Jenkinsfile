pipeline {
  agent {
    docker {
      image 'gcc:5.4'
    }

  }
  stages {
    stage('Build') {
      steps {
        sh '''mkdir build
mkdir lib
cd src
make -j4'''
      }
    }
    stage('Test') {
      steps {
        sh '''cd src
./gtest'''
      }
    }
  }
}