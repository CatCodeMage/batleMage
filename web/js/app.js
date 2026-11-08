document.addEventListener('DOMContentLoaded', async () => {
  try {
    await loadAllData();
  } catch (e) {
    document.body.innerHTML = `<p style="color:red">Error cargando datos: ${e.message}</p>`;
    return;
  }

  renderStep(1);
  bindNav();
});

function bindNav() {
  document.getElementById('btn-next').addEventListener('click', () => {
    if (!validateStep(State.currentStep)) return;
    if (State.currentStep < 5) {
      State.currentStep++;
      renderStep(State.currentStep);
    }
  });

  document.getElementById('btn-prev').addEventListener('click', () => {
    if (State.currentStep > 1) {
      State.currentStep--;
      renderStep(State.currentStep);
    }
  });
}

function renderStep(n) {
  document.querySelectorAll('.step').forEach(s => s.classList.remove('active'));
  document.querySelectorAll('#stepper span').forEach(s => {
    s.classList.toggle('active', Number(s.dataset.step) === n);
    s.classList.toggle('done', Number(s.dataset.step) < n);
  });

  document.getElementById(`step-${n}`).classList.add('active');
  document.getElementById('btn-prev').disabled = (n === 1);
  document.getElementById('btn-next').textContent = (n === 5) ? 'Revisar →' : 'Siguiente →';

  ({
    1: renderStep1,
    2: renderStep2,
    3: renderStep3,
    4: renderStep4,
    5: renderStep5,
  })[n]();
}

function validateStep(n) {
  return ({
    1: () => validateStep1(),
    2: () => validateStep2(),
    3: () => validateStep3(),
    4: () => validateStep4(),
    5: () => true,
  })[n]();
}